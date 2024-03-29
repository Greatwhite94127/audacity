#include "stdio.h"
#ifndef mips
#include "stdlib.h"
#endif
#include "xlisp.h"
#include "sound.h"

#include "falloc.h"
#include "cext.h"
#include "slope.h"

void slope_free();


typedef struct slope_susp_struct {
    snd_susp_node susp;
    long terminate_cnt;
    boolean logically_stopped;
    sound_type input;
    long input_cnt;
    sample_block_values_type input_ptr;

    sample_type prev;
    double scale;
} slope_susp_node, *slope_susp_type;


void slope_n_fetch(register slope_susp_type susp, snd_list_type snd_list)
{
    int cnt = 0; /* how many samples computed */
    int togo;
    int n;
    sample_block_type out;
    register sample_block_values_type out_ptr;

    register sample_block_values_type out_ptr_reg;

    register sample_type prev_reg;
    register double scale_reg;
    register sample_block_values_type input_ptr_reg;
    falloc_sample_block(out, "slope_n_fetch");
    out_ptr = out->samples;
    snd_list->block = out;

    while (cnt < max_sample_block_len) { /* outer loop */
    /* first compute how many samples to generate in inner loop: */
    /* don't overflow the output sample block: */
    togo = max_sample_block_len - cnt;

    /* don't run past the input input sample block: */
    susp_check_term_log_samples(input, input_ptr, input_cnt);
    togo = min(togo, susp->input_cnt);

    /* don't run past terminate time */
    if (susp->terminate_cnt != UNKNOWN &&
        susp->terminate_cnt <= susp->susp.current + cnt + togo) {
        togo = susp->terminate_cnt - (susp->susp.current + cnt);
        if (togo == 0) break;
    }


    /* don't run past logical stop time */
    if (!susp->logically_stopped && susp->susp.log_stop_cnt != UNKNOWN) {
        int to_stop = susp->susp.log_stop_cnt - (susp->susp.current + cnt);
        /* break if to_stop == 0 (we're at the logical stop)
         * AND cnt > 0 (we're not at the beginning of the
         * output block).
         */
        if (to_stop < togo) {
        if (to_stop == 0) {
            if (cnt) {
            togo = 0;
            break;
            } else /* keep togo as is: since cnt == 0, we
                    * can set the logical stop flag on this
                    * output block
                    */
            susp->logically_stopped = true;
        } else /* limit togo so we can start a new
                * block at the LST
                */
            togo = to_stop;
        }
    }

    n = togo;
    prev_reg = susp->prev;
    scale_reg = susp->scale;
    input_ptr_reg = susp->input_ptr;
    out_ptr_reg = out_ptr;
    if (n) do { /* the inner sample computation loop */
{ register sample_type x = *input_ptr_reg++;
*out_ptr_reg++ = (sample_type) ((x - prev_reg) * scale_reg);
prev_reg = x;};
    } while (--n); /* inner loop */

    susp->prev = prev_reg;
    /* using input_ptr_reg is a bad idea on RS/6000: */
    susp->input_ptr += togo;
    out_ptr += togo;
    susp_took(input_cnt, togo);
    cnt += togo;
    } /* outer loop */

    /* test for termination */
    if (togo == 0 && cnt == 0) {
    snd_list_terminate(snd_list);
    } else {
    snd_list->block_len = cnt;
    susp->susp.current += cnt;
    }
    /* test for logical stop */
    if (susp->logically_stopped) {
    snd_list->logically_stopped = true;
    } else if (susp->susp.log_stop_cnt == susp->susp.current) {
    susp->logically_stopped = true;
    }
} /* slope_n_fetch */


void slope_toss_fetch(susp, snd_list)
  register slope_susp_type susp;
  snd_list_type snd_list;
{
    long final_count = susp->susp.toss_cnt;
    time_type final_time = susp->susp.t0;
    long n;

    /* fetch samples from input up to final_time for this block of zeros */
    while ((round((final_time - susp->input->t0) * susp->input->sr)) >=
       susp->input->current)
    susp_get_samples(input, input_ptr, input_cnt);
    /* convert to normal processing when we hit final_count */
    /* we want each signal positioned at final_time */
    n = round((final_time - susp->input->t0) * susp->input->sr -
         (susp->input->current - susp->input_cnt));
    susp->input_ptr += n;
    susp_took(input_cnt, n);
    susp->susp.fetch = susp->susp.keep_fetch;
    (*(susp->susp.fetch))(susp, snd_list);
}


void slope_mark(slope_susp_type susp)
{
    sound_xlmark(susp->input);
}


void slope_free(slope_susp_type susp)
{
    sound_unref(susp->input);
    ffree_generic(susp, sizeof(slope_susp_node), "slope_free");
}


void slope_print_tree(slope_susp_type susp, int n)
{
    indent(n);
    stdputstr("input:");
    sound_print_tree_1(susp->input, n);
}


sound_type snd_make_slope(sound_type input)
{
    register slope_susp_type susp;
    rate_type sr = input->sr;
    time_type t0 = input->t0;
    int interp_desc = 0;
    sample_type scale_factor = 1.0F;
    time_type t0_min = t0;
    falloc_generic(susp, slope_susp_node, "snd_make_slope");
    susp->prev = 0.0F;
    susp->scale = input->sr * input->scale;
    susp->susp.fetch = slope_n_fetch;
    susp->terminate_cnt = UNKNOWN;
    /* handle unequal start times, if any */
    if (t0 < input->t0) sound_prepend_zeros(input, t0);
    /* minimum start time over all inputs: */
    t0_min = min(input->t0, t0);
    /* how many samples to toss before t0: */
    /* Toss an extra 1 samples to make up for internal buffering: */
    susp->susp.toss_cnt = (long) ((t0 - t0_min) * sr + 1.5);
    if (susp->susp.toss_cnt > 0) {
    susp->susp.keep_fetch = susp->susp.fetch;
    susp->susp.fetch = slope_toss_fetch;
    }

    /* initialize susp state */
    susp->susp.free = slope_free;
    susp->susp.sr = sr;
    susp->susp.t0 = t0;
    susp->susp.mark = slope_mark;
    susp->susp.print_tree = slope_print_tree;
    susp->susp.name = "slope";
    susp->logically_stopped = false;
    susp->susp.log_stop_cnt = logical_stop_cnt_cvt(input);
    susp->susp.current = 0;
    susp->input = input;
    susp->input_cnt = 0;
    return sound_create((snd_susp_type)susp, t0, sr, scale_factor);
}


sound_type snd_slope(sound_type input)
{
    sound_type input_copy = sound_copy(input);
    return snd_make_slope(input_copy);
}
