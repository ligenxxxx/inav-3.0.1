function(setup_svd target_exe target_name)
    get_property(svd_name TARGET ${target_exe} PROPERTY SVD)
    set(svd_target_name "svd_${target_name}")
    if (svd_name AND NOT svd_name STREQUAL "")
        add_custom_target(${svd_target_name}
            COMMAND ${CMAKE_COMMAND} -E copy
            ${SVD_DIR}/${svd_name}.svd
            ${CMAKE_BINARY_DIR}/svd/${target_name}.svd
        )
    else()
        add_custom_target(${svd_target_name}
            ${CMAKE_COMMAND} -E echo "target ${target_name} does not declare an SVD filename"
            COMMAND ${CMAKE_COMMAND} -E false)
    endif()
    exclude_from_all(${svd_target_name})
endfunction()
