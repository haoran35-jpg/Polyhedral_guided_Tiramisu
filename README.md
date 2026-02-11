# Polyhedral-Guided Tiramisu

Constraint-based search space reduction for polyhedral auto-scheduling.

## Quick Start

```bash
git clone --recursive https://github.com/haoran35-jpg/Polyhedral_guided_Tiramisu.git
cd Polyhedral_guided_Tiramisu

# Build dependencies
cd external/pluto && ./autogen.sh && ./configure && make -j4 && cd ../..
cd external/tiramisu && mkdir -p build && cd build
cmake -DUSE_AUTO_SCHEDULER=ON .. && make -j4 && cd ../../..

# Build project
mkdir build && cd build && cmake .. && make -j4
```

## Run

```bash
cd build
./benchmark_search_space_comparison
```

## License

MIT
