list(APPEND srcs "averager/averager.cpp" "keys/keys.cpp" "split/split.cpp" )
idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "averager/include" "keys/include" "split/include")

