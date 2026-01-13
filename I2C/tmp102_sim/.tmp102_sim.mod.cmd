savedcmd_tmp102_sim.mod := printf '%s\n'   tmp102_sim.o | awk '!x[$$0]++ { print("./"$$0) }' > tmp102_sim.mod
