cmd_/home/stu/lab3/Module.symvers :=  sed 's/ko$$/o/'  /home/stu/lab3/modules.order | scripts/mod/modpost -m -a     -o /home/stu/lab3/Module.symvers -e -i Module.symvers -T - 
