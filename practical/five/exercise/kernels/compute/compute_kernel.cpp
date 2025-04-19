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

    unary_op_init_common(cb_in0, cb_out0);
    add_binary_tile_init();
    copy_tile_to_dst_init_short(cb_in0);

    for (uint32_t i=0;i<num_chunks;i++) {
        // Wait for a block of tiles in each of input CBs
        cb_wait_front(cb_in0, 1);
        cb_wait_front(cb_in1, 1);

        // Aquire dst registers for compute core
        tile_regs_acquire();

        // TODO: Copy the tile in the cb_in0 CB into a segment of the
        // dst register, this is done via the copy_tile API call
        // with the CB as the first argument, the CB's page (zero)
        // as the second argument, and the third argument is the 
        // dst regsiter segment (0 to 15 inclusing, 0 is suggested here)
        
        // TODO: Copy the tile in the cb_in1 CB into a segment of the
        // dst register, this is done via the copy_tile API call
        // with the CB as the first argument, the CB's page (zero)
        // as the second argument, and the third argument is the 
        // dst regsiter segment (0 to 15 inclusing, 1 is suggested here)

        // TODO: Direct the SFPU to add the tiles in dst register segments
        // that the CBs were copied into in the above two calls. The API
        // call is add_binary_tile, with the two source dst register segment
        // indexes provided as arguments. Like all SFPU calls, results are
        // written into the first dst register segment provided as an argument.        

        // Commit the dst registers so they can be consumed
        tile_regs_commit();

        // Pop pages in the input CBs so these can be reused
        cb_pop_front(cb_in0, 1);
        cb_pop_front(cb_in1, 1);

        // Reserve a page in the output CB
        cb_reserve_back(cb_out0, 1);

        // Wait for dst registers for packer RV core
        tile_regs_wait();
        // Pack from segement 0 in the dst register to the output CB
        pack_tile(0, cb_out0);
        // Release dst registers
        tile_regs_release();

        // Make output tile available to consumer
        cb_push_back(cb_out0, 1);
    }
}
}

