// ============================================================================
// Message Routing Performance Benchmarks
// ============================================================================
// Measures performance of port-based message operations:
// - Message creation and allocation
// - Port metadata generation (compile-time machinery)
// - Type-based message routing throughput
// - Memory allocation patterns

#include <benchmark/benchmark.h>
#include <graph/PortTypes.hpp>
#include <graph/Message.hpp>
#include <memory>
#include <vector>

using namespace graph;
using graph::message::Message;

// ============================================================================
// Message Creation and Memory Allocation
// ============================================================================

static void BM_Message_Creation_Throughput(benchmark::State& state) {
    // Measure message allocation and creation throughput
    std::vector<Message> messages;
    messages.reserve(100);

    for (auto _ : state) {
        for (int i = 0; i < 100; i++) {
            messages.push_back(Message());
        }
        benchmark::DoNotOptimize(messages);
        messages.clear();
    }

    state.counters["messages/sec"] = benchmark::Counter(
        state.iterations() * 100,
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}
BENCHMARK(BM_Message_Creation_Throughput)->Iterations(10000);

static void BM_Message_Copy_Throughput(benchmark::State& state) {
    // Measure cost of message copying
    Message msg;

    std::vector<Message> copies;
    copies.reserve(100);

    for (auto _ : state) {
        for (int i = 0; i < 100; i++) {
            copies.push_back(msg);  // Copy message
        }
        benchmark::DoNotOptimize(copies);
        copies.clear();
    }

    state.counters["copies/sec"] = benchmark::Counter(
        state.iterations() * 100,
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}
BENCHMARK(BM_Message_Copy_Throughput)->Iterations(100000);

// ============================================================================
// Port Operations
// ============================================================================

static void BM_Port_MetadataGeneration(benchmark::State& state) {
    // Port metadata is compile-time only (no runtime cost)
    // This benchmark measures any template instantiation overhead
    for (auto _ : state) {
        // Instantiate different port specializations (with actual Message type)
        using Port0 = Port<Message, 0>;
        using Port1 = Port<Message, 1>;
        using Port2 = Port<Message, 2>;

        benchmark::DoNotOptimize(Port0::id);
        benchmark::DoNotOptimize(Port1::id);
        benchmark::DoNotOptimize(Port2::id);
    }
}
BENCHMARK(BM_Port_MetadataGeneration)->Iterations(1000000);

// ============================================================================
// Routing Performance
// ============================================================================

static void BM_Message_TypeBasedRouting_Lookup(benchmark::State& state) {
    // Measure type-based message routing lookup overhead
    // (simulating port selection based on message type)

    std::vector<int> port_ids;
    port_ids.reserve(100);
    for (int i = 0; i < 100; i++) {
        port_ids.push_back(i % 10);  // 10 possible ports
    }

    int lookup_count = 0;

    for (auto _ : state) {
        for (int port_id : port_ids) {
            benchmark::DoNotOptimize(port_id);
            lookup_count++;
        }
    }

    state.counters["lookups/sec"] = benchmark::Counter(
        lookup_count,
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}
BENCHMARK(BM_Message_TypeBasedRouting_Lookup)->Iterations(100000);

// ============================================================================
// Memory Allocation Patterns
// ============================================================================

static void BM_Message_AllocationRate(benchmark::State& state) {
    // Measure heap allocation rate for message creation
    int num_allocations = 0;
    std::vector<Message> messages;
    messages.reserve(state.range(0));

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); i++) {
            messages.push_back(Message());
            num_allocations++;
        }

        benchmark::DoNotOptimize(messages);
        messages.clear();
    }

    state.counters["alloc/sec"] = benchmark::Counter(
        num_allocations,
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}
BENCHMARK(BM_Message_AllocationRate)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Iterations(1000);

// ============================================================================
// Message Queue Throughput (simulated routing)
// ============================================================================

static void BM_MessageQueue_Throughput(benchmark::State& state) {
    // Measure throughput of message queuing through multiple ports
    int num_ports = state.range(0);
    std::vector<std::vector<Message>> port_queues(num_ports);

    for (auto _ : state) {
        for (int msg_id = 0; msg_id < 1000; msg_id++) {
            Message msg;
            int port = msg_id % num_ports;
            port_queues[port].push_back(msg);
        }

        // Clear queues
        for (auto& queue : port_queues) {
            queue.clear();
        }
    }

    state.counters["messages/sec"] = benchmark::Counter(
        state.iterations() * 1000,
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}
BENCHMARK(BM_MessageQueue_Throughput)
    ->Arg(1)
    ->Arg(4)
    ->Arg(16)
    ->Iterations(1000);

// ============================================================================
// Port Index Access
// ============================================================================

static void BM_Port_IndexAccess(benchmark::State& state) {
    // Measure compile-time port index access
    for (auto _ : state) {
        for (int i = 0; i < 100; i++) {
            benchmark::DoNotOptimize(Port<Message, 0>::id);
            benchmark::DoNotOptimize(Port<Message, 1>::id);
            benchmark::DoNotOptimize(Port<Message, 2>::id);
            benchmark::DoNotOptimize(Port<Message, 3>::id);
        }
    }

    state.counters["accesses/sec"] = benchmark::Counter(
        state.iterations() * 100 * 4,
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}
BENCHMARK(BM_Port_IndexAccess)->Iterations(100000);

BENCHMARK_MAIN();
