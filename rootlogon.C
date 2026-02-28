{
  gSystem->AddIncludePath("${PUEO_UTIL_INSTALL_DIR}/include/pueo");
  gSystem->Load("${PUEO_UTIL_INSTALL_DIR}/lib/libpueorawdata.so");

  gSystem->AddIncludePath("${PUEO_UTIL_INSTALL_DIR}/include/NiceMC");
  gInterpreter->AddIncludePath("inc/");
}
