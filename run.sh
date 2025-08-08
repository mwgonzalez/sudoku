if [ $(whoami) != "root" ]; then echo RUN AS ROOT && exit; fi

trap ':' INT

# Minimize non-determinism for reproducible results
# Run at highest priority (SCHED_FIFO 99)
# Run on single CPU (avoid cache ping pong)
# Pin CPU frequency (avoid turbo boost & thermal throttling)

echo -1 > /proc/sys/kernel/sched_rt_runtime_us
for I in 0 1 2 3; do echo userspace > /sys/devices/system/cpu/cpu$I/cpufreq/scaling_governor; done
for I in 0 1 2 3; do echo   2800000 > /sys/devices/system/cpu/cpu$I/cpufreq/scaling_setspeed; done
sleep 0.5

chrt -f 99 taskset -c 3 "$@"

for I in 0 1 2 3; do echo schedutil > /sys/devices/system/cpu/cpu$I/cpufreq/scaling_governor; done
echo 950000 > /proc/sys/kernel/sched_rt_runtime_us
