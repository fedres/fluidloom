#include <benchmark/benchmark.h>
#include "fluidloom/transport/MPITransport.h"
#include "fluidloom/transport/GPUAwareBuffer.h"
#include "fluidloom/core/backend/MockBackend.h"
#include <mpi.h>
#include <memory>

using namespace fluidloom;
using namespace fluidloom::transport;

class TransportBenchmark : public benchmark::Fixture {
protected:
    std::unique_ptr<MockBackend> backend;
    std::unique_ptr<MPITransport> transport;
    int rank;
    int size;
    
    void SetUp(const benchmark::State& state) override {
        (void)state;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        
        backend = std::make_unique<MockBackend>();
        backend->initialize(rank);
        transport = std::make_unique<MPITransport>(backend.get());
    }
    
    void TearDown(const benchmark::State& state) override {
        (void)state;
        transport.reset();
        backend->shutdown();
    }
};

// Benchmark MPI_Isend latency (target: < 1ms)
BENCHMARK_DEFINE_F(TransportBenchmark, SendLatency)(benchmark::State& state) {
    size_t buffer_size = state.range(0);
    auto buffer = createGPUAwareBuffer(backend.get(), buffer_size);
    int partner_rank = (rank == 0) ? 1 : 0;
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        auto req = transport->send_async(partner_rank, buffer.get(), 0, buffer_size, 0);
        auto end = std::chrono::high_resolution_clock::now();
        
        req->wait();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6); // Convert to seconds
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * buffer_size);
}
BENCHMARK_REGISTER_F(TransportBenchmark, SendLatency)
    ->Args({1024})      // 1KB
    ->Args({4096})      // 4KB
    ->Args({16384})     // 16KB
    ->Args({65536})     // 64KB
    ->UseManualTime()
    ->Unit(benchmark::kMicrosecond);

// Benchmark MPI_Irecv latency
BENCHMARK_DEFINE_F(TransportBenchmark, RecvLatency)(benchmark::State& state) {
    size_t buffer_size = state.range(0);
    auto buffer = createGPUAwareBuffer(backend.get(), buffer_size);
    int partner_rank = (rank == 0) ? 1 : 0;
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        auto req = transport->recv_async(partner_rank, buffer.get(), 0, buffer_size, 0);
        auto end = std::chrono::high_resolution_clock::now();
        
        req->wait();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * buffer_size);
}
BENCHMARK_REGISTER_F(TransportBenchmark, RecvLatency)
    ->Args({1024})
    ->Args({4096})
    ->Args({16384})
    ->Args({65536})
    ->UseManualTime()
    ->Unit(benchmark::kMicrosecond);

// Benchmark round-trip exchange
BENCHMARK_DEFINE_F(TransportBenchmark, RoundTripExchange)(benchmark::State& state) {
    size_t buffer_size = state.range(0);
    auto send_buffer = createGPUAwareBuffer(backend.get(), buffer_size);
    auto recv_buffer = createGPUAwareBuffer(backend.get(), buffer_size);
    int partner_rank = (rank == 0) ? 1 : 0;
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto send_req = transport->send_async(partner_rank, send_buffer.get(), 0, buffer_size, 0);
        auto recv_req = transport->recv_async(partner_rank, recv_buffer.get(), 0, buffer_size, 0);
        
        send_req->wait();
        recv_req->wait();
        
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * buffer_size * 2); // Send + Recv
}
BENCHMARK_REGISTER_F(TransportBenchmark, RoundTripExchange)
    ->Args({1024})
    ->Args({4096})
    ->Args({16384})
    ->Args({65536})
    ->Args({262144})    // 256KB
    ->Args({1048576})   // 1MB
    ->UseManualTime()
    ->Unit(benchmark::kMicrosecond);

// Benchmark message rate (messages/sec)
BENCHMARK_DEFINE_F(TransportBenchmark, MessageRate)(benchmark::State& state) {
    size_t buffer_size = 1024; // Fixed 1KB messages
    auto send_buffer = createGPUAwareBuffer(backend.get(), buffer_size);
    auto recv_buffer = createGPUAwareBuffer(backend.get(), buffer_size);
    int partner_rank = (rank == 0) ? 1 : 0;
    
    int message_count = 0;
    for (auto _ : state) {
        auto send_req = transport->send_async(partner_rank, send_buffer.get(), 0, buffer_size, message_count);
        auto recv_req = transport->recv_async(partner_rank, recv_buffer.get(), 0, buffer_size, message_count);
        
        send_req->wait();
        recv_req->wait();
        
        message_count++;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["messages_per_sec"] = benchmark::Counter(
        state.iterations(), 
        benchmark::Counter::kIsRate
    );
}
BENCHMARK_REGISTER_F(TransportBenchmark, MessageRate)
    ->Unit(benchmark::kMicrosecond);

// Main function
int main(int argc, char** argv) {
    // Initialize MPI
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    
    // Initialize benchmark
    ::benchmark::Initialize(&argc, argv);
    
    // Run benchmarks
    ::benchmark::RunSpecifiedBenchmarks();
    
    // Cleanup
    ::benchmark::Shutdown();
    MPI_Finalize();
    
    return 0;
}
