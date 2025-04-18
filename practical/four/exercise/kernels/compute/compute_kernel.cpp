#include "compute_kernel_api/eltwise_binary.h"
#include "compute_kernel_api/tile_move_copy.h"

namespace NAMESPACE {
void MAIN {
    uint32_t data_size = get_arg_val<uint32_t>(0);
    uint32_t chunk_size = get_arg_val<uint32_t>(1);

    constexpr auto cb_in0 = tt::CBIndex::c_0;
    constexpr auto cb_in1 = tt::CBIndex::c_1;
    constexpr auto cb_out0 = tt::CBIndex::c_2;

    uint32_t num_chunks = data_size / chunk_size;
    
    binary_op_init_common(cb_in0, cb_in1, cb_out0);
    add_tiles_init(cb_in0, cb_in1);

    for (uint32_t i=0;i<num_chunks;i++) {
        // Wait for a block of tiles in each of input CBs
        cb_wait_front(cb_in0, 1);
        cb_wait_front(cb_in1, 1);

        // TODO: Aquire dst registers for compute core by tile_regs_acquire API call
        
        // TODO: Add the tiles via the matrix multiplication engine from page 0 
        // in both CB and into segment 0 of dst registers by the add_tiles API
        // call where  the two CBs are the first two arguments, and the page
        // location provided as the next two arguments (as zero) and the location
        // in the dst register provided as the last argument, suggest zero
        
        // TODO: Commit the dst registers so they can be consumed via tile_regs_commit API call        

        // Pop pages in the input CBs so these can be reused
        cb_pop_front(cb_in0, 1);
        cb_pop_front(cb_in1, 1);

        // Reserve a page in the output CB
        cb_reserve_back(cb_out0, 1);

        // TODO: Wait for dst registers for packer RV core via the tile_regs_wait API call
        
        // TODO: Pack from segement 0 in the dst register to the output CB via the
        // pack_tile API call, with the dst register segment index (what was provided as the
        // last argument in add_tiles) and the output CB index, cb_out0
        
        // TODO: Release dst registers via the tile_regs_release API call        

        // Make output tile available to consumer
        cb_push_back(cb_out0, 1);
    }
}
}

