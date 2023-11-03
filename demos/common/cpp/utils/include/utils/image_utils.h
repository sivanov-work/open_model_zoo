/*
// Copyright (C) 2021-2023 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once
#include <tuple>

#include <opencv2/opencv.hpp>

enum RESIZE_MODE {
    RESIZE_FILL,
    RESIZE_KEEP_ASPECT,
    RESIZE_KEEP_ASPECT_LETTERBOX
};

cv::Mat resizeImageExt(const cv::Mat& mat, int width, int height, RESIZE_MODE resizeMode = RESIZE_FILL,
                       cv::InterpolationFlags interpolationMode = cv::INTER_LINEAR, cv::Rect* roi = nullptr,
                       cv::Scalar BorderConstant = cv::Scalar(0, 0, 0));


bool is_aspect_ratio_equal(const std::tuple<int, int> &lhs_res,const std::tuple<int, int> &rhs_res);
