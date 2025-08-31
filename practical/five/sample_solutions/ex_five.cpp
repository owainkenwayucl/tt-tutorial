#include "host_api.hpp"
#include "device.hpp"

#define DATA_SIZE 65536
#define CHUNK_SIZE 1024

using namespace tt;
using namespace tt::tt_metal;

int main(int argc, char** argv) {
    // Create device handle
    IDevice* device = CreateDevice(0);

    // Setup program to execute along with its buffers and kernels to use
    CommandQueue& cq = device->command_queue();
    Program program = CreateProgram();
    constexpr CoreCoord core = {0, 0};

    // Create descriptor of DRAM allocation
    constexpr uint32_t ddr_tile_size = 4 * DATA_SIZE;
    InterleavedBufferConfig dram_config{
        .device = device, .size = ddr_tile_size, .page_size = ddr_tile_size, .buffer_type = BufferType::DRAM};

    // Use descriptor configuration to allocate buffers in DRAM on the device
    std::shared_ptr<Buffer> src0_dram_buffer = CreateBuffer(dram_config);
    std::shared_ptr<Buffer> src1_dram_buffer = CreateBuffer(dram_config);
    std::shared_ptr<Buffer> dst_dram_buffer = CreateBuffer(dram_config);

    constexpr uint32_t l1_tile_size = 4 * CHUNK_SIZE;
    // Create L1 circular buffers to communicate between RV cores
    CircularBufferConfig cb_src0_config =
        CircularBufferConfig(l1_tile_size, {{CBIndex::c_0, tt::DataFormat::Int32}})
            .set_page_size(CBIndex::c_0, l1_tile_size);
    tt_metal::CreateCircularBuffer(program, core, cb_src0_config);

    CircularBufferConfig cb_src1_config =
        CircularBufferConfig(l1_tile_size, {{CBIndex::c_1, tt::DataFormat::Int32}})
            .set_page_size(CBIndex::c_1, l1_tile_size);
    tt_metal::CreateCircularBuffer(program, core, cb_src1_config);

    CircularBufferConfig cb_src2_config =
        CircularBufferConfig(l1_tile_size, {{CBIndex::c_2, tt::DataFormat::Int32}})
            .set_page_size(CBIndex::c_2, l1_tile_size);
    tt_metal::CreateCircularBuffer(program, core, cb_src2_config);

    // Allocate input data and fill it with values (each will be added together)
    int32_t * src0_data=(int32_t*) malloc(sizeof(int32_t) * DATA_SIZE);
    int32_t * src1_data=(int32_t*) malloc(sizeof(int32_t) * DATA_SIZE);

    for (int i=0;i<DATA_SIZE;i++) {
        src0_data[i]=i;
        src1_data[i]=DATA_SIZE-i;
    }

    // Intentionally set incorrect values in the result buffer, this ensures that
    // previously correct results are not read later (if the kernel did not run)
    EnqueueWriteBuffer(cq, dst_dram_buffer, src0_data, true);

    // Write the src0 and src1 data to DRAM on the device, false means we will not block
    EnqueueWriteBuffer(cq, src0_dram_buffer, src0_data, false);
    EnqueueWriteBuffer(cq, src1_dram_buffer, src1_data, false);

    // Specify data movement kernel for launching on first RISC-V baby core
    KernelHandle reader_kernel_id = CreateKernel(
        program,
        "kernels/dataflow/read_kernel.cpp",
        core,
        DataMovementConfig{.processor = DataMovementProcessor::RISCV_0, .noc = NOC::RISCV_0_default});

    // Configure reader runtime kernel arguments
    SetRuntimeArgs(
        program,
        reader_kernel_id,
        core,
        {src0_dram_buffer->address(),
         src1_dram_buffer->address(),
         DATA_SIZE,
         CHUNK_SIZE});

    // Specify data movement kernel for launching on last RISC-V baby core
    KernelHandle writer_kernel_id = CreateKernel(
        program,
        "kernels/dataflow/write_kernel.cpp",
        core,
        DataMovementConfig{.processor = DataMovementProcessor::RISCV_1, .noc = NOC::RISCV_1_default});

    // Configure writer runtime kernel arguments
    SetRuntimeArgs(
        program,
        writer_kernel_id,
        core,
        {dst_dram_buffer->address(),
         DATA_SIZE,
         CHUNK_SIZE});

    // Compute kernel creation
    KernelHandle compute_kernel_id = CreateKernel(
        program,
        "kernels/compute/compute_kernel.cpp",
        core,
        ComputeConfig{
            .math_fidelity = MathFidelity::HiFi4,
            .fp32_dest_acc_en = false,
            .math_approx_mode = false,
            .compile_args = {},
        });

    // Configure compute runtime kernel arguments
    SetRuntimeArgs(
        program,
        compute_kernel_id,
        core,
        {DATA_SIZE,
         CHUNK_SIZE});

    // Enqueue the program to run on the device, false means will not block    
    EnqueueProgram(cq, program, false);
    // Wait on the command queue to complete
    Finish(cq);

    // Allocate result data on host for results
    // Copy results back from device DRAM, true means will block for completion
    int32_t * result_data=(int32_t*) malloc(sizeof(int32_t) * DATA_SIZE);
    EnqueueReadBuffer(cq, dst_dram_buffer, result_data, true);

    int number_failures=0;
    for (int i=0;i<DATA_SIZE;i++) {
        if (result_data[i] != src0_data[i] + src1_data[i]) number_failures++;
    }

    CloseDevice(device);

    if (number_failures==0) {
        printf("Completed successfully on the device, with %d elements\n", DATA_SIZE);
    } else {
        printf("Failure on the device, %d fails with %d elements\n", number_failures, DATA_SIZE);
    }
}

