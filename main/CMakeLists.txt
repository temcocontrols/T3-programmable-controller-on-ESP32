idf_component_register(SRCS "tcp_server.c"
							"modbus.c"
							"deviceparams.c"
							"i2c_task.c"
							"ota.c"
							"microphone.c"
							"wifi.c"
							"sgp30_featureset.c"
							"sensirion_common.c"
							"sgp30.c"
							"ethernet_task.c"
							"flash.c"
							"pro_info.c"
							"scd40.c"
                    		INCLUDE_DIRS ".")
#                    		EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem)

target_link_libraries(${COMPONENT_TARGET} "-u pro_info")
