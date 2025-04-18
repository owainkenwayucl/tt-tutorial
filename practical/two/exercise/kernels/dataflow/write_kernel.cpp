#include "dataflow_api.h"

#define DATA_TYPE_BYTES 4

void kernel_main() {
    uint32_t dst_addr = get_arg_val<uint32_t>(0);
    uint32_t data_size = get_arg_val<uint32_t>(1);

    // Get destination DRAM NoC address for writing
    uint64_t dst_noc_addr = get_noc_addr_from_bank_id<true>(0, dst_addr);

    // Circular buffer index used to communicate data from the other data mover core
    constexpr uint32_t cb_id_out0 = tt::CBIndex::c_0;

    // Bytes that are written to DDR for the data
    uint32_t bytes_data_size=DATA_TYPE_BYTES * data_size;

    // TODO: Wait for a page to be made available in the CB, you do this
    // via the cb_wait_front API call, with the CB identifier, cb_id_out0, 
    // as the first argument and 1 as the second argument (as we are waiting
    // on one page being made available).

    // Now we have the page grab the read address of this
    uint32_t l1_read_addr = get_read_ptr(cb_id_out0);
    
    // Write the recieved data to DDR
    noc_async_write(l1_read_addr, dst_noc_addr, bytes_data_size);
    noc_async_write_barrier();

    // TODO: Pop the page to make it available for writing again, you do thi
    // via the cb_pop_front API call, with the CB identifier, cb_id_out0,
    // as the first argument and 1 as the second argument (as we are releasing
    // one page).
}

