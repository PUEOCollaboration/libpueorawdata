{
  gSystem->AddIncludePath("${PUEO_UTIL_INSTALL_DIR}/include/pueorawdata/pueo");
  gSystem->Load("${PUEO_UTIL_INSTALL_DIR}/lib64/libpueorawdata.so");

  gSystem->AddIncludePath("${PUEO_UTIL_INSTALL_DIR}/include/NiceMc");
  gSystem->Load("${PUEO_UTIL_INSTALL_DIR}/lib64/libNiceMC.so");
  gInterpreter->AddIncludePath("inc/");
}
