# Resource decoder for encrypted Pixel Game Maker MV games. Apply patches/ to
# the Linux player, then build it with -DPGMMV_DECRYPTION_DIR=<this directory>.
# On its own, this builds the library and its tests.

cmake_minimum_required(VERSION 3.16)

project(pgmmv-linux-decryption CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(NETTLE REQUIRED IMPORTED_TARGET nettle)

add_library(pgmmv_decryption STATIC src/EncryptedResources.cpp)
target_include_directories(pgmmv_decryption PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(pgmmv_decryption PUBLIC cxx_std_11)
target_link_libraries(pgmmv_decryption PUBLIC PkgConfig::NETTLE)

include(CTest)
if(BUILD_TESTING)
    add_executable(pgmmv_decryption_tests tests/EncryptedResourcesTest.cpp)
    target_link_libraries(pgmmv_decryption_tests PRIVATE pgmmv_decryption)
    add_test(NAME encrypted_resources COMMAND pgmmv_decryption_tests)
endif()

include(GNUInstallDirs)
install(FILES LICENSE RENAME pgmmv-linux-decryption-LICENSE.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pgmmv-player/licenses)
install(FILES licenses/pgmm_decrypt-LICENSE.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pgmmv-player/licenses)
