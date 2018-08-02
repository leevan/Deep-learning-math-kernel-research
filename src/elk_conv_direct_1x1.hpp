#ifndef __ELK_CONV_DIRECT_1X1_HPP__
#define __ELK_CONV_DIRECT_1X1_HPP__

#include <boost/preprocessor/seq/for_each_product.hpp>
#include <boost/preprocessor/seq/to_tuple.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include "el_def.hpp"
#include "el_utils.hpp"
#include "elx_conv.hpp"

#define MAX_FMA_PRL 36

namespace euler {

// Type: data type
// O2: OC blocking
// T: tile blocking unit
// V: vector size
// I: ISA
// with_bias: has bias
// with_relu: with relu fusion
// with_sum: with sum fusion

#define BIAS(x) x
#define BORDER(x) x
#define RELU(x) x
#define SUM(x) x

template <typename Type, const int O2, const int T, const int V, const int I,
    const bool with_bias, const bool with_relu, const bool with_sum>
struct convolution_direct_1x1_kernel {
  static void gemm(elx_conv_t<Type> &xc, Type *output, Type *input,
      Type *weights, Type *bias);
  static void gemm_tail(elx_conv_t<Type> &xc, Type *output, Type *input,
      Type *weights, Type *bias);
};

#define DEF_convolution_direct_1x1_kernel(z, T, nil)                           \
  template <typename Type, const int V, const int I, const bool with_bias,     \
      const bool with_relu, const bool with_sum>                               \
  struct convolution_direct_1x1_kernel<Type, 1, T, V, I, with_bias, with_relu, \
      with_sum> {                                                              \
    static void gemm(elx_conv_t<Type> &xc, Type *output, Type *input,          \
        Type *weights, Type *bias);                                            \
    static void gemm_tail(elx_conv_t<Type> &xc, Type *output, Type *input,     \
        Type *weights, Type *bias);                                            \
  };

BOOST_PP_REPEAT_FROM_TO(1, MAX_FMA_PRL, DEF_convolution_direct_1x1_kernel, nil);


} // namespace euler


#endif // __ELK_CONV_DIRECT_1X1_HPP__
