cmd_/home/stu/lab3/lab3_driver.mod := printf '%s\n'   lab3_driver.o | awk '!x[$$0]++ { print("/home/stu/lab3/"$$0) }' > /home/stu/lab3/lab3_driver.mod
