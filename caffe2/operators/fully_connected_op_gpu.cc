/**
 * Copyright (c) 2016-present, Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "caffe2/core/common_gpu.h"
#include "caffe2/core/context_gpu.h"
#include "caffe2/operators/fully_connected_op.h"

namespace caffe2 {

namespace {

constexpr int kFp16CUDADevicePropMajor = 6;

template <class FullyConnectedOp>
bool RunFullyConnectedOpOnCUDADevice(
    const bool float16_compute,
    FullyConnectedOp* op) {
  if (op->Input(0).template IsType<float>()) {
    return op->template DoRunWithType<
        float, // X
        float, // W
        float, // B
        float, // Y
        float>(); // Math
  } else if (op->Input(0).template IsType<float16>()) {
    if (float16_compute) {
      const cudaDeviceProp& prop = GetDeviceProperty(0);
      if (prop.major >= kFp16CUDADevicePropMajor) {
        return op->template DoRunWithType<
            float16, // X
            float16, // W
            float16, // B
            float16, // Y
            float16>(); // Math
      } else {
        LOG(INFO) << "CUDA Device does not support FP16 computation, "
                     "falling back to FP32.";
        return op->template DoRunWithType<
            float16, // X
            float16, // W
            float16, // B
            float16, // Y
            float>(); // Math
      }
    } else {
      return op->template DoRunWithType<
          float16, // X
          float16, // W
          float16, // B
          float16, // Y
          float>(); // Math
    }
  } else {
    CAFFE_THROW("Unsupported type");
  }
  return false;
}

template <class FullyConnectedGradientOp>
bool RunFullyConnectedGradientOpOnCUDADevice(
    const bool float16_compute,
    FullyConnectedGradientOp* op) {
  if (op->Input(0).template IsType<float>()) {
    return op->template DoRunWithType<
        float, //  X
        float, //  W
        float, // dY
        float, //  B
        float, // dX
        float, // dW
        float, // dB
        float>(); // Math
  } else if (op->Input(0).template IsType<float16>()) {
    if (float16_compute) {
      const cudaDeviceProp& prop = GetDeviceProperty(0);
      if (prop.major >= kFp16CUDADevicePropMajor) {
        return op->template DoRunWithType<
            float16, //  X
            float16, //  W
            float16, // dY
            float16, //  B
            float16, // dX
            float16, // dW
            float16, // dB
            float16>(); // Math
      } else {
        LOG(INFO) << "CUDA Device does not support FP16 computation, "
                     "falling back to FP32.";
        return op->template DoRunWithType<
            float16, //  X
            float16, //  W
            float16, // dY
            float16, //  B
            float16, // dX
            float16, // dW
            float16, // dB
            float>(); // Math
      }
    } else {
      return op->template DoRunWithType<
          float16, //  X
          float16, //  W
          float16, // dY
          float16, //  B
          float16, // dX
          float16, // dW
          float16, // dB
          float>(); // Math
    }
  } else {
    CAFFE_THROW("Unsupported type");
  }
  return false;
}

} // namespace

// The RunFullyConnectedOpOnCUDADevice Function will use the pointer of current
// op and the DoRunWithType will make sure to run the correct things.
template <>
bool FullyConnectedOp<CUDAContext>::RunOnDevice() {
  return RunFullyConnectedOpOnCUDADevice(float16_compute_, this);
}

template <>
bool FullyConnectedOp<
    CUDAContext,
    DefaultEngine,
    false /* don't transpose weight */>::RunOnDevice() {
  return RunFullyConnectedOpOnCUDADevice(float16_compute_, this);
}

template <>
bool FullyConnectedGradientOp<CUDAContext>::RunOnDevice() {
  return RunFullyConnectedGradientOpOnCUDADevice(float16_compute_, this);
}

template <>
bool FullyConnectedGradientOp<
    CUDAContext,
    DefaultEngine,
    false /* don't transpose weight */>::RunOnDevice() {
  return RunFullyConnectedGradientOpOnCUDADevice(float16_compute_, this);
}

#if CUDA_VERSION >= 9000

// Require these to be defined otherwise TensorCore FC ops will end
// up calling the default FC implementation which doesn't have
// fp16 support...

template <>
bool FullyConnectedOp<CUDAContext, TensorCoreEngine>::RunOnDevice() {
  return RunFullyConnectedOpOnCUDADevice(false /* float16_compute */, this);
}

template <>
bool FullyConnectedOp<
    CUDAContext,
    TensorCoreEngine,
    false /* don't transpose weight */>::RunOnDevice() {
  return RunFullyConnectedOpOnCUDADevice(false /* float16_compute */, this);
}

template <>
bool FullyConnectedGradientOp<CUDAContext, TensorCoreEngine>::RunOnDevice() {
  return RunFullyConnectedGradientOpOnCUDADevice(
      false /* float16_compute */, this);
}

template <>
bool FullyConnectedGradientOp<
    CUDAContext,
    TensorCoreEngine,
    false /* don't transpose weight */>::RunOnDevice() {
  return RunFullyConnectedGradientOpOnCUDADevice(
      false /* float16_compute */, this);
}

#endif

REGISTER_CUDA_OPERATOR(FC, FullyConnectedOp<CUDAContext>);
REGISTER_CUDA_OPERATOR(FCGradient, FullyConnectedGradientOp<CUDAContext>);

REGISTER_CUDA_OPERATOR(
    FCTransposed,
    FullyConnectedOp<
        CUDAContext,
        DefaultEngine,
        false /* don't transpose weight */>);
REGISTER_CUDA_OPERATOR(
    FCTransposedGradient,
    FullyConnectedGradientOp<
        CUDAContext,
        DefaultEngine,
        false /* don't transpose weight */>);

#if CUDA_VERSION >= 9000
REGISTER_CUDA_OPERATOR_WITH_ENGINE(
    FC,
    TENSORCORE,
    FullyConnectedOp<CUDAContext, TensorCoreEngine>);
REGISTER_CUDA_OPERATOR_WITH_ENGINE(
    FCGradient,
    TENSORCORE,
    FullyConnectedGradientOp<CUDAContext, TensorCoreEngine>);

REGISTER_CUDA_OPERATOR_WITH_ENGINE(
    FCTransposed,
    TENSORCORE,
    FullyConnectedOp<
        CUDAContext,
        TensorCoreEngine,
        false /* don't transpose weight */>);
REGISTER_CUDA_OPERATOR_WITH_ENGINE(
    FCTransposedGradient,
    TENSORCORE,
    FullyConnectedGradientOp<
        CUDAContext,
        TensorCoreEngine,
        false /* don't transpose weight */>);
#endif

} // namespace caffe2
