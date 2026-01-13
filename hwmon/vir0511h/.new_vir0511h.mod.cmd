savedcmd_new_vir0511h.mod := printf '%s\n'   new_vir0511h.o | awk '!x[$$0]++ { print("./"$$0) }' > new_vir0511h.mod
