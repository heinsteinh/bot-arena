# Renderer v0.3 — Job System & Thread-Local Arenas

v0.3 parallelizes render-command *generation*. A reusable `JobSystem` thread pool
fans command building out across worker lanes; each lane owns a thread-local
`Arena` + `RenderQueue`, so workers record commands with no locks. After
generation the per-lane buffers are merged and sorted, then executed on the main
thread. Only command generation is parallel — OpenGL stays single-threaded.

See `docs/renderer-v0.2.md` for the mesh path and sort keys this builds on.

## Data Flow: One Frame

```
Application  ── owns ──► JobSystem (fixed thread pool)
     └── owns ──► Renderer(JobSystem&)
                    │  lanes[0..workerCount-1] = {Arena, RenderQueue}
                    ▼
BotArenaGame::onRender
  │  serial:   grid + wall meshes  →  renderer.queue()  (lane 0)
  │  parallel: renderer.generateMeshes(kBotCount, batchFn)
  │              → JobSystem::parallelFor splits [0,N) into batches
  │                each batch runs on a lane; batchFn writes that lane's queue
  ▼   (Renderer::endFrame)
  merge: append every lane's entries → std::stable_sort by [layer|shader|material|depth]
  backend.execute(merged, viewProjection, lane0.arena, registry)   // main thread
  ▼   next beginFrame: reset every lane's arena + clear its queue
GPU
```

Workers only build POD commands into their own arena — no locks, no OpenGL. This
is the Molecule "workers write command buffers, the main thread executes" split.

## JobSystem

`engine/core/JobSystem.hpp` is a fixed thread pool:

```cpp
explicit JobSystem(std::optional<unsigned> workerThreads = std::nullopt);
size_t workerCount() const;   // worker threads + 1 (the main lane); always >= 1
void parallelFor(size_t count, size_t batchSize,
                 const std::function<void(size_t begin, size_t end, size_t lane)>& fn);
```

- Default (`nullopt`) spawns `hardware_concurrency() - 1` worker threads; `0`
  forces serial (0 workers) for tests; `N` uses N workers.
- **Lane 0 is the calling (main) thread**; worker threads are lanes `1..N`. Every
  lane has a stable index used to select a thread-local buffer.
- `parallelFor` is a barrier: it enqueues batches, wakes workers, and the main
  thread also drains batches (as lane 0) before waiting on a completion counter.
  With `workerCount() == 1` it simply runs everything on lane 0 — the deterministic
  serial fallback that CI, single-core machines, and the headless screenshot path
  take.

## Thread-Local Command Buffers

`engine/renderer/CommandBufferPool.hpp` defines one buffer per lane:

```cpp
struct WorkerBuffer {
  explicit WorkerBuffer(std::size_t bytes) : arena(bytes), queue(arena) {}
  Arena arena;
  RenderQueue queue;
};
```

The `Renderer` owns `std::vector<Scope<WorkerBuffer>>` sized to
`jobs.workerCount()` (heap-boxed because `Arena` is non-movable), each with an
8 MiB arena. `queue()` returns lane 0's queue for serial producers.
`generateMeshes(count, fn)` runs `fn(lane.queue, begin, end)` on each lane via
`parallelFor` — every lane records into its own arena, so there is no shared
mutable state and no false sharing between workers.

`Renderer::endFrame` concatenates the lanes with `mergeLaneEntries`, then
`std::stable_sort`s the merged `RenderEntry` list and hands it to the backend.
Each `RenderEntry::command` points into its lane's arena, which lives until the
next `beginFrame`, so the merged list and its `execute` are all valid within the
frame.

## Determinism

Batch-to-lane scheduling is nondeterministic (whichever lane grabs a batch runs
it), so the *order* of equal-or-colliding sort keys in the merged list can vary
run to run. That does not change the image: every draw here is opaque and
depth-tested, which is order-independent, and the grid lines are identical. The
rendered frame is therefore deterministic despite parallel scheduling.
(Transparency would need explicit ordering — a later milestone.)

## Registry Safety

Materials and the unit-cube mesh are registered once on the main thread (the
game's first frame, and `Renderer::initBuiltins`) before any `parallelFor`.
During generation the `ResourceRegistry` is read-only, so concurrent
`MeshRenderer` reads across workers are safe.

## The Bot Swarm

`BotArenaGame` renders `kBotCount = 2048` small cubes inside the arena walls.
Each bot's transform is a pure function of `(index, time)` (a position on an
animated lattice), so generation needs no shared per-bot state. The swarm is
produced with:

```cpp
renderer.generateMeshes(kBotCount, [&](RenderQueue& q, size_t begin, size_t end) {
  MeshRenderer mesh(q, renderer.registry(), *camera);
  for (size_t i = begin; i < end; ++i)
    mesh.submit(cube, m_swarmMats[i % 3], botTransform(i, time));
});
```

The walls and grid are still submitted serially on lane 0; the swarm fans out
across all lanes and merges before the sort.

## Testing

- `JobSystem`: `parallelFor` visits every index exactly once (atomic count + sum),
  lane indices stay `< workerCount()`, `count == 0` is a no-op, the serial path
  (`workerThreads == 0`) still covers everything, and a 50k-slot write test detects
  lost/duplicated writes from any race.
- `mergeLaneEntries`: concatenates lane entry lists in order.
- Behavioral: the `BOTARENA_SCREENSHOT` path renders the full swarm without
  crashing.

## Next Milestones

- **v0.4** — `UniformBuffer`, `Texture`, `Framebuffer`, `RenderPass` for
  offscreen/HDR rendering.
- **Later** — GPU instancing so the whole swarm becomes a single draw call;
  AI and physics systems consuming the same `JobSystem`.
