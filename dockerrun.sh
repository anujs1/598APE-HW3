docker run -it --rm \
  --cap-add=SYS_ADMIN \
  --cap-add=SYS_PTRACE \
  --cap-add=PERFMON \
  --security-opt seccomp=unconfined \
  -v "$PWD":/host \
  -w /host \
  anujs1/598ape \
  bash
