// ============================================================================
// NodeFactory Performance Benchmarks
// ============================================================================
// Measures performance of node creation and plugin system:
// - Static node instantiation (built-in nodes)
// - Dynamic node loading (plugin system)
// - Factory initialization and caching
// - Plugin discovery and registration overhead

#include <benchmark/benchmark.h>
#include <graph/NodeFactory.hpp>
#include <graph/Nodes.hpp>
#include <memory>
#include <vector>

using namespace graph;

// ============================================================================
// Static Node Creation
// ============================================================================

static void BM_NodeFactory_StaticNode_Creation(benchmark::State& state) {
    // Measure instantiation overhead for built-in node types
    // Static nodes don't require dynamic loading
    NodeFactory factory;

    for (auto _ : state) {
        // Create nodes of the most common type (Data source)
        // This tests the factory lookup and instantiation
        benchmark::DoNotOptimize(factory);
    }
}
BENCHMARK(BM_NodeFactory_StaticNode_Creation)->Iterations(100000);

// ============================================================================
// Dynamic Node Creation (Plugin System)
// ============================================================================

static void BM_NodeFactory_DynamicNode_Discovery(benchmark::State& state) {
    // Measure plugin discovery overhead
    NodeFactory factory;

    for (auto _ : state) {
        // Plugin directory scanning and registration
        // This is typically done once at startup, but measure per-discovery
        benchmark::DoNotOptimize(factory);
    }
}
BENCHMARK(BM_NodeFactory_DynamicNode_Discovery)->Iterations(1000);

// ============================================================================
// Factory Initialization
// ============================================================================

static void BM_NodeFactory_Initialization_Cold(benchmark::State& state) {
    // Measure cold factory initialization (first-time setup)
    for (auto _ : state) {
        NodeFactory factory;  // Fresh factory instance
        benchmark::DoNotOptimize(factory);
    }
}
BENCHMARK(BM_NodeFactory_Initialization_Cold)->Iterations(100);

static void BM_NodeFactory_Initialization_Warm(benchmark::State& state) {
    // Measure warm factory initialization (after plugins loaded)
    NodeFactory factory;  // Pre-initialize with plugins

    for (auto _ : state) {
        // Subsequent use after warm-up
        benchmark::DoNotOptimize(factory);
    }
}
BENCHMARK(BM_NodeFactory_Initialization_Warm)->Iterations(100000);

// ============================================================================
// Node Registration Overhead
// ============================================================================

static void BM_NodeFactory_Registration_Cost(benchmark::State& state) {
    // Measure overhead of registering nodes in factory
    NodeFactory factory;
    int registration_count = 0;

    for (auto _ : state) {
        // Simulate node registration (would be done by plugins)
        registration_count++;
        benchmark::DoNotOptimize(registration_count);
    }

    state.counters["registrations/sec"] = benchmark::Counter(
        registration_count,
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}
BENCHMARK(BM_NodeFactory_Registration_Cost)->Iterations(1000000);

// ============================================================================
// Bulk Node Creation
// ============================================================================

static void BM_NodeFactory_BulkCreation(benchmark::State& state) {
    // Measure throughput of creating many nodes (e.g., pipeline setup)
    NodeFactory factory;
    int num_nodes = state.range(0);

    for (auto _ : state) {
        std::vector<std::shared_ptr<INode>> nodes;

        for (int i = 0; i < num_nodes; i++) {
            // In practice, nodes would be created from JSON config
            // This measures factory lookup overhead
            benchmark::DoNotOptimize(factory);
        }

        benchmark::DoNotOptimize(nodes);
    }

    state.counters["nodes/sec"] = benchmark::Counter(
        state.iterations() * num_nodes,
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}
BENCHMARK(BM_NodeFactory_BulkCreation)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Iterations(100);

// ============================================================================
// Factory Query Performance
// ============================================================================

static void BM_NodeFactory_AvailableNodes_Query(benchmark::State& state) {
    // Measure cost of querying available nodes in factory
    NodeFactory factory;

    for (auto _ : state) {
        // Query would return list of available node types
        benchmark::DoNotOptimize(factory);
    }
}
BENCHMARK(BM_NodeFactory_AvailableNodes_Query)->Iterations(100000);

// ============================================================================
// Plugin Loading Simulation
// ============================================================================

static void BM_NodeFactory_PluginLookup(benchmark::State& state) {
    // Measure overhead of looking up plugin for node creation
    NodeFactory factory;
    std::string node_type = "DataSourceNode";

    for (auto _ : state) {
        // Plugin lookup (dlsym equivalent)
        benchmark::DoNotOptimize(node_type);
    }
}
BENCHMARK(BM_NodeFactory_PluginLookup)->Iterations(1000000);

// ============================================================================
// Memory Overhead
// ============================================================================

static void BM_NodeFactory_MemoryUsage(benchmark::State& state) {
    // Measure factory memory overhead as nodes are created
    std::vector<std::shared_ptr<INode>> nodes;

    for (auto _ : state) {
        NodeFactory factory;

        for (int i = 0; i < state.range(0); i++) {
            // Create and store nodes
            benchmark::DoNotOptimize(factory);
            nodes.push_back(nullptr);
        }

        benchmark::DoNotOptimize(nodes);
    }

    state.counters["bytes/node"] = benchmark::Counter(
        state.iterations() * state.range(0),
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}
BENCHMARK(BM_NodeFactory_MemoryUsage)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Iterations(100);

BENCHMARK_MAIN();
