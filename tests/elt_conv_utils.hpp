#include <type_traits>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "el_utils.hpp"
#include "elt_utils.hpp"
#include "euler.hpp"

#ifndef __ELT_CONV_UTILS_HPP__
#define __ELT_CONV_UTILS_HPP__

namespace euler {
namespace test {
  enum {
    FP32 = 0,
    FP16,
    FP16O,
    U8F32F32F32,
    U8F32U8F32,
    U8F32S8F32
  };

  template <typename InputType, typename WeightsType, typename OutputType, typename BiasType>
  void prepare_conv_data(eld_conv_t &desc_ref, eld_conv_t &desc,
      float *input_ref, float *weights_ref, float *output_ref, float *bias_ref,
      InputType **input, WeightsType **weights, OutputType **output, BiasType **bias,
      const char *input_file, const char *weights_file, const char *bias_file,
      int input_format, int weights_format, bool double_buffering = false,
      int data_type_cfg = 0, bool f16c_opt = false, bool validate_results = false);

  int __compare_conv_results_nchw(eld_conv_t &, float *out,
      float *ref, int data_type_cfg, double acc);

  int __compare_conv_results_nhwc(eld_conv_t &, float *out,
      float *ref, int data_type_cfg, double acc);

  int __compare_conv_results_blocked(eld_conv_t &, float *out,
      float *ref, int data_type_cfg, double acc);

  int compare_conv_results(eld_conv_t &, float *out, float *ref,
      int data_type_cfg, bool is_int8_lp = false, bool with_real_data = false);

  void post_process_conv_results(float *ouput_ref, eld_conv_t &desc,
      void *output_res, int data_type_cfg);

  size_t cal_ops(eld_conv_t &desc);
  int cal_iterations(size_t num_ops);

  template <typename Type, const int dst_fmt, const int src_fmt,
      typename... Args>
  struct reorder {
    reorder(Type *dst, Type *src, Args...)
    {
      assert(dst != nullptr && src != nullptr);
    }
  };

  template <typename Type> struct reorder<Type, nchw, nhwc> {
    reorder(Type *dst, Type *src, int n, int c, int h, int w);
  };

  template <typename Type> struct reorder<Type, nhwc, nchw> {
    reorder(Type *dst, Type *src, int n, int c, int h, int w);
  };

  template <typename Type> struct reorder<Type, nchw, nChw16c> {
    reorder(Type *dst, Type *src, int n, int c, int h, int w);
  };

  template <typename Type> struct reorder<Type, nChw16c, nchw> {
    reorder(Type *dst, Type *src, int n, int c, int h, int w);
  };

  template <typename Type> struct reorder<Type, oihw, OIhw16i16o> {
    reorder(Type *dst, Type *src, int o, int i, int h, int w);
  };

  template <typename Type> struct reorder<Type, goihw, gOIhw16i16o> {
    reorder(Type *dst, Type *src, int g, int o, int i, int h, int w);
  };

  template <typename Type> struct reorder<Type, OIhw16i16o, oihw> {
    reorder(Type *dst, Type *src, int o, int i, int h, int w);
  };

  template <typename Type> struct reorder<Type, gOIhw16i16o, goihw> {
    reorder(Type *dst, Type *src, int g, int o, int i, int h, int w);
  };

  template <typename Type> struct reorder<Type, oihw, hwio> {
    reorder(Type *dst, Type *src, int o, int i, int h, int w);
  };

  template <typename Type> struct reorder<Type, goihw, ghwio> {
    reorder(Type *dst, Type *src, int g, int o, int i, int h, int w);
  };

  template <typename Type> struct reorder<Type, hwio, oihw> {
	reorder(Type *dst, Type *src, int o, int i, int h, int w);
  };

  template <typename Type> struct reorder<Type, ghwio, goihw> {
	reorder(Type *dst, Type *src, int g, int o, int i, int h, int w);
  };

  template <typename InputType, typename WeightsType, typename OutputType, typename BiasType>
  int ref_conv_deconv_2d(eld_conv_t &desc,
      OutputType *output, InputType *input, WeightsType *weights, BiasType *bias);

  template <typename InputType, typename WeightsType, typename OutputType, typename BiasType>
  int ref_convolution2d(eld_conv_t &desc,
      OutputType *output, InputType *input, WeightsType *weights, BiasType *bias);
}
}

#endif // __ELT_CONV_UTILS_HPP__
