SRC_DIR= policy_server media_server registrar_server proxy_server #

include makefile.comm

.PHONY:srv
srv:
	@for mk_file in $(SRC_DIR); do echo "making $$mk_file"; $(MAKE) -C $$mk_file; done
clean:
	@for mk_file in $(SRC_DIR); do echo "making clean  $$mk_file"; $(MAKE) clean -C $$mk_file; done
