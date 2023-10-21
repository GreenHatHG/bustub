# 1.build
```shell
docker run --rm --user $UID:$GID -v "$(pwd)":/home/bustub -w /home/bustub bustub bash -c "rm -rf build && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j$(nproc)"
```

# 2.format
```shell
docker run --rm --user $UID:$GID -v "$(pwd)":/home/bustub -w /home/bustub bustub bash -c "cd build && make format && make check-lint && make check-clang-tidy-p2"
```