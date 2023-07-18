// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "custom_kernels.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>

#include <ie_core.hpp>
#include <ie_data.h>
#include <ie_layouts.h>
#include <opencv2/gapi/core.hpp>
#include <opencv2/gapi/cpu/gcpukernel.hpp>
#include <opencv2/gapi/gscalar.hpp>
#include <opencv2/gapi/imgproc.hpp>
#include <opencv2/gapi/infer.hpp>
#include <opencv2/gapi/operators.hpp>
#include <opencv2/gapi/own/assert.hpp>
#include <opencv2/imgproc.hpp>


void IndexScore::getScoredLabels(const std::vector<std::string> &in_labes,
                                 LabelsStorage &out_scored_labels_to_append) const {
    // fill starting from max confidence
    for (auto conf_index_it = max_confidence_with_indices.rbegin();
         conf_index_it != max_confidence_with_indices.rend();
         ++conf_index_it) {
        try {
            out_scored_labels_to_append.emplace_back(conf_index_it->first,
                                                     *conf_index_it->second,
                                                     in_labes.at(*conf_index_it->second));
        } catch (const std::out_of_range& ex) {
            throw std::out_of_range(std::string("Provided labels file doesn't contain classified label index\nException: ") + ex.what());
        }
    }
}

IndexScore IndexScore::create_from_array(const float *out_blob_data_ptr, size_t out_blob_element_count,
                                         size_t top_k_amount) {
    IndexScore ret;
    if (!out_blob_data_ptr) {
        return IndexScore();
    }
    // find top K
    size_t i = 0;
    // fill & sort topK with first K elements of N array: O(K*Log(K))
    while(i < std::min(top_k_amount, out_blob_element_count)) {
        ret.max_element_indices.push_back(i);
        ret.max_confidence_with_indices.emplace(out_blob_data_ptr[i], std::prev(ret.max_element_indices.end()));
        i++;
    }

    // search K elements through remnant N-K array elements
    // greater than the minimum element in the pivot topK
    // O((N-K)*Log(K))
    for (i = top_k_amount; i < out_blob_element_count && !ret.max_confidence_with_indices.empty(); i++) {
        const auto &low_confidence_it = ret.max_confidence_with_indices.begin();
        if (out_blob_data_ptr[i] >= low_confidence_it->first) {
            auto list_min_elem_it = low_confidence_it->second;
            *list_min_elem_it = i;
            ret.max_confidence_with_indices.erase(low_confidence_it);
            ret.max_confidence_with_indices.emplace(out_blob_data_ptr[i], list_min_elem_it);
        }
    }
    return ret;
}

GAPI_OCV_KERNEL(OCVLocateROI, custom::LocateROI) {
    // This is the place where we can run extra analytics
    // on the input image frame and select the ROI (region
    // of interest) where we want to classify our objects (or
    // run any other inference).
    //
    // Crops the input image to square (this is
    // the most convenient aspect ratio for detectors to use)

    static void run(const cv::Size& in_size,
                    cv::Rect &out_rect) {

        // Identify the central point & square size (- some padding)
        const auto center = cv::Point{in_size.width/2, in_size.height/2};
        auto sqside = std::min(in_size.width, in_size.height);

        // Now build the central square ROI
        out_rect = cv::Rect{ center.x - sqside/2
                             , center.y - sqside/2
                             , sqside
                             , sqside
                            };
    }
};

GAPI_OCV_KERNEL(OCVTopK, custom::TopK) {
    static void run(const cv::MediaFrame &in, const cv::Mat &out_blob, uint32_t top_k_amount, IndexScore &out) {
        cv::MatSize out_blob_size = out_blob.size;
        if (out_blob_size.dims() != 2) {
            throw std::runtime_error(std::string("Incorrect inference result blob dimensions has been detected: ") +
                                     std::to_string(out_blob_size.dims()) + ", expected: 2 for classification networks");
        }

        if (out_blob.type() != CV_32F) {
            throw std::runtime_error(std::string("Incorrect inference result blob elements type has been detected: ") +
                                     std::to_string(out_blob.type()) + ", expected: CV_32F for classification networks");
        }
        const float *out_blob_data_ptr = out_blob.ptr<float>();
        const size_t out_blob_data_elem_count = out_blob.total();

        out = IndexScore::create_from_array(out_blob_data_ptr, out_blob_data_elem_count, top_k_amount);
    }
};

cv::gapi::GKernelPackage custom::kernels() {
    return cv::gapi::kernels<OCVLocateROI, OCVTopK>();
}
