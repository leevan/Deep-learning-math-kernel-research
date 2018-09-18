#pragma once
#include <assert.h>
#include <x86intrin.h>
#include "elk_def.hpp"
#include "el_def.hpp"
#include "el_utils.hpp"
#include "elx_conv.hpp"
#include "elk_conv_wino.hpp"
#include "elk_conv_wino_2x2_3x3_input_gen.hxx"

namespace euler {

// template <const bool is_border_, const bool with_bias_>
// Params:
//   elx_conv_t<float> &xc,
//   float *output, float atoutput[A][A][V], float *bias,
//   int _hOA_end, int _wOA_end
 template <bool ...conditions>
inline void convolution_winograd_kernel_base<float, ISA_GENERIC, 16, 4, 3>::
__trans_output(elx_conv_t<float> &xc, float *output,
      float atoutput[A][A][V], float *bias, int hOA_end, int wOA_end) {
  float dummy[16];
  constexpr bool is_border = cd_traits<conditions...>::is_border;
  constexpr bool with_bias = cd_traits<conditions...>::with_bias;
  constexpr bool with_relu = cd_traits<conditions...>::with_relu;
  constexpr bool with_ip_sum = cd_traits<conditions...>::with_ip_sum;
  bool fuse_ip_sum = with_ip_sum && (wOA_end != -1);
  bool fuse_bias = with_bias && (bias != nullptr);
  bool fuse_relu = with_relu && (bias != nullptr);

  auto p_cb = [&](int _h, int _w, int _V) {
    if (wOA_end == -1) {
      MD3(float, aoutput, output, A - K + 1, A - K + 1, V);
      return &md3(aoutput, _h, _w, _V);
    } else {
      MD3(float, aoutput, output, xc.oh, xc.ow, V);
      if (is_border && (_h > hOA_end || _w > wOA_end))
        return &dummy[_V];
      else
        return &md3(aoutput, _h, _w, _V);
    }
  };

#undef T
#undef C
#undef P
#undef B
#undef S
#define T(_hA, _wA) atoutput[_wA][_hA][_V]
#define C(n) c##n[_V]
#define S(n) s##n[_V]
#define P(_h, _w) *p_cb(_h, _w, _V)
#define B bias[_V]
  float c0[V], c1[V], c2[V], c3[V];
  float s0[V], s1[V], s2[V], s3[V];
#pragma omp simd
  for (int _V = 0; _V < V; ++_V) {
    C(0) = T(1,0) + T(1,1) + T(1,2);
    C(1) = T(2,0) + T(2,1) + T(2,2);
    C(2) = T(1,2) + T(1,3) - T(1,1);
    C(3) = T(2,2) + T(2,3) - T(2,1);

    S(0) = T(0,0) + T(0,1) + T(0,2) + C(0) + C(1);
    if (fuse_bias) S(0) += B;
    if (fuse_ip_sum) S(0) += P(0, 0);
    if (fuse_relu) S(0) = S(0) > 0 ? S(0) : 0;
    P(0, 0) = S(0);
    S(1) = C(1) - C(0) + T(3,0) + T(3,1) + T(3,2);
    if (fuse_bias) S(1) += B;
    if (fuse_ip_sum) S(1) += P(1, 0);
    if (fuse_relu) S(1) = S(1) > 0 ? S(1) : 0;
    P(1, 0) = S(1);
    S(2) = T(0,2) - T(0,1) + T(0,3) + C(2) + C(3);
    if (fuse_bias) S(2) += B;
    if (fuse_ip_sum) S(2) += P(0, 1);
    if (fuse_relu) S(2) = S(2) > 0 ? S(2) : 0;
    P(0, 1) = S(2);
    S(3) = C(3) - C(2) - T(3,1) + T(3,2) + T(3,3);
    if (fuse_bias) S(3) += B;
    if (with_ip_sum) S(3) += P(1, 1);
    if (fuse_relu) S(3) = S(3) > 0 ? S(3) : 0;
    P(1, 1) = S(3);
  }
}

// Params:
//   elx_conv_t<float> &xc, float *toutputa, float *toutput, int Tz,
//   bool stream_out
 template <bool ...conditions>
inline void convolution_winograd_kernel_base<float, ISA_GENERIC, 16, 4, 3>::
__trans_outputa_th(elx_conv_t<float> &xc, float *toutputa, float *toutput,
    int Tz, bool stream_out) {

  MD4(float, atoutput, toutput, A, xc.oc3 * xc.O2, Tz, V);
  MD2(float, atoutputa, toutputa, A - K + 1, V);

#undef P
#undef T
#define T(_h) md4(atoutput, _h, 0, 0, _V)
#define P(_h) md2(atoutputa, _h, _V)

#pragma omp simd
  for (int _V = 0; _V < V; ++_V) {
    P(0) = T(0) + T(1) + T(2);
    P(1) = - T(1) + T(2) + T(3);
  }
}


#define GENERIC_CALCULATE_TILE_4(z, n, nil)              \
  S(0) = T(n, 0) + T(n, 1) + T(n, 2);                    \
  if (fuse_bias) S(0) += B;                              \
  if (fuse_ip_sum) S(0) += P(n, 0);                      \
  if (fuse_relu) S(0) = S(0) > 0 ? S(0) : 0;             \
  P(n, 0) = S(0);                                        \
  S(1) = T(n, 2) - T(n, 1) + T(n, 3);                    \
  if (fuse_bias) S(1) += B;                              \
  if (fuse_ip_sum) S(1) += P(n, 1);                      \
  if (fuse_relu) S(1) = S(1) > 0 ? S(1) : 0;             \
  P(n, 1) = S(1);

// template <const bool is_border_, const bool with_bias_>
// Params:
//   elx_conv_t<float> &xc,
//   float *output, float atoutput[A][A - K + 1][V], float *bias,
//   int _hOA_end, int _wOA_end
template <bool ...conditions>
inline void convolution_winograd_kernel_base<float, ISA_GENERIC, 16, 4, 3>::
__trans_outputa_bh(elx_conv_t<float> &xc, float *output,
    float atoutput[A][A - K + 1][V], float *bias, int hOA_end, int wOA_end) {
  constexpr bool is_border = cd_traits<conditions...>::is_border;
  constexpr bool with_bias = cd_traits<conditions...>::with_bias;
  constexpr bool with_relu = cd_traits<conditions...>::with_relu;
  constexpr bool with_ip_sum = cd_traits<conditions...>::with_ip_sum;
  bool fuse_ip_sum = with_ip_sum && (wOA_end != -1);
  bool fuse_bias = with_bias && (bias != nullptr);
  bool fuse_relu = with_relu && (bias != nullptr);

   float dummy[16];
   auto p_cb = [&](int _h, int _w, int _V) {
     if (wOA_end == -1) {
       MD3(float, aoutput, output, A - K + 1, A - K + 1, V);
       return &md3(aoutput, _h, _w, _V);
     } else {
       MD3(float, aoutput, output, xc.oh, xc.ow, V);
       if (is_border && (_h > hOA_end || _w > wOA_end))
         return &dummy[_V];
       else
         return &md3(aoutput, _h, _w , _V);
     }
   };

#undef T
#undef C
#undef P
#undef B
#define S(n) s##n[_V]
#define T(_hA, _wA) atoutput[_wA][_hA][_V]
#define P(_h, _w) *p_cb(_h, _w, _V)
#define B bias[_V]

  float s0[V], s1[V];
#pragma omp simd
  for (int _V = 0; _V < V; ++_V) {
    BOOST_PP_REPEAT(2, GENERIC_CALCULATE_TILE_4, nil)
  }

}

} // namespace euler
