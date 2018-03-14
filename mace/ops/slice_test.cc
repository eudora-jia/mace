//
// Copyright (c) 2017 XiaoMi All rights reserved.
//

#include "mace/ops/slice.h"
#include "mace/ops/ops_test_util.h"
#include "gmock/gmock.h"

using namespace mace;

class SliceOpTest : public OpsTestBase {};

template<DeviceType D, typename T>
void RandomTest(const int num_outputs) {
  srand(time(nullptr));
  const index_t output_channels = 4 * (1 + rand() % 10);
  const index_t input_channels = num_outputs * output_channels;
  const index_t batch = 3 + (rand() % 10);
  const index_t height = 13 + (rand() % 10);
  const index_t width = 17 + (rand() % 10);

  // Construct graph
  OpsTestNet net;

  std::vector<index_t> input_shape({batch, height, width, input_channels});
  const index_t input_size = std::accumulate(input_shape.begin(), input_shape.end(), 1, std::multiplies<index_t>());
  std::vector<float> input_data(input_size);
  GenerateRandomRealTypeData(input_shape, input_data);
  net.AddInputFromArray<D, float>("Input", input_shape, input_data);

  if (D == DeviceType::OPENCL) {
    BufferToImage<D, T>(net, "Input", "InputImage",
                        kernels::BufferType::IN_OUT_CHANNEL);

    auto builder = OpDefBuilder("Slice", "SliceTest");
    builder.Input("InputImage");
    for (int i = 0; i < num_outputs; ++i) {
      builder = builder.Output(MakeString("OutputImage", i));
    }
    builder
        .AddIntArg("T", static_cast<int>(DataTypeToEnum<T>::value))
        .Finalize(net.NewOperatorDef());
  } else {
    auto builder = OpDefBuilder("Slice", "SliceTest");
    builder.Input("Input");
    for (int i = 0; i < num_outputs; ++i) {
      builder = builder.Output(MakeString("Output", i));
    }
    builder.Finalize(net.NewOperatorDef());

  }

  // Run
  net.RunOp(D);

  if (D == DeviceType::OPENCL) {
    for (int i = 0; i < num_outputs; ++i) {
      ImageToBuffer<D, float>(net, MakeString("OutputImage", i), MakeString("Output", i),
                              kernels::BufferType::IN_OUT_CHANNEL);
    }
  }

  // Check
  std::vector<index_t> expected_shape({batch, height, width, output_channels});
  const index_t outer_size = std::accumulate(expected_shape.begin(), expected_shape.end() - 1,
                                             1, std::multiplies<index_t>());
  const float *input_ptr = input_data.data();
  const float *output_ptr;
  for (int i = 0; i < num_outputs; ++i) {
    auto output = net.GetOutput(MakeString("Output", i).c_str());
    EXPECT_THAT(output->shape(), ::testing::ContainerEq(expected_shape));
    Tensor::MappingGuard output_mapper(output);
    output_ptr = output->data<float>();
    for (int outer_idx = 0; outer_idx < outer_size; ++outer_idx) {
      const int idx = outer_idx * input_channels + i * output_channels;
      for (int j = 0; j < output_channels; ++j) {
        ASSERT_NEAR(*output_ptr++, input_ptr[idx + j], 1e-2) << "with output " << i << " index " << idx + j;
      }
    }
  }
}

TEST_F(SliceOpTest, CPU) {
  RandomTest<DeviceType::CPU, float>(2);
  RandomTest<DeviceType::CPU, float>(4);
  RandomTest<DeviceType::CPU, float>(11);
}

TEST_F(SliceOpTest, OPENCLFloat) {
  RandomTest<DeviceType::OPENCL, float>(2);
  RandomTest<DeviceType::OPENCL, float>(4);
  RandomTest<DeviceType::OPENCL, float>(11);
}

TEST_F(SliceOpTest, OPENCLHalf) {
  RandomTest<DeviceType::OPENCL, half>(2);
  RandomTest<DeviceType::OPENCL, half>(4);
  RandomTest<DeviceType::OPENCL, half>(11);
}