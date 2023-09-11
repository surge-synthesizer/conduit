
find_package(Git)

if( EXISTS ${CONDUITSRC}/VERSION_GIT_INFO )
  message( STATUS "VERSION_GIT_INFO file is present; using that rather than git query" )
  # Line 2 is the branch, line 3 is the hash
  execute_process(
    COMMAND sed -n "2p" ${CONDUITSRC}/VERSION_GIT_INFO
    WORKING_DIRECTORY ${CONDUITSRC}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  execute_process(
    COMMAND sed -n "3p" ${CONDUITSRC}/VERSION_GIT_INFO
    WORKING_DIRECTORY ${CONDUITSRC}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

elseif( Git_FOUND )
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CONDUITSRC}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CONDUITSRC}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

if("${GIT_BRANCH}" STREQUAL "")
  message(WARNING "Could not determine Git branch, using placeholder.")
  set(GIT_BRANCH "git-no-branch")
endif()
if ("${GIT_COMMIT_HASH}" STREQUAL "")
  message(WARNING "Could not determine Git commit hash, using placeholder.")
  set(GIT_COMMIT_HASH "git-no-commit")
endif()

if( WIN32 )
  set( CONDUIT_BUILD_ARCH "Intel" )
else()
  execute_process(
    COMMAND uname -m
    OUTPUT_VARIABLE CONDUIT_BUILD_ARCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

cmake_host_system_information(RESULT CONDUIT_BUILD_FQDN QUERY FQDN )

message( STATUS "Setting up Conduit version:" )
message( STATUS "  Git hash is ${GIT_COMMIT_HASH} and branch is ${GIT_BRANCH}" )
message( STATUS "  Build host is ${CONDUIT_BUILD_FQDN}" )
message( STATUS "  Build architecture is ${CONDUIT_BUILD_ARCH}" )

if( ${AZURE_PIPELINE} )
  message( STATUS "Azure pipeline build" )
  set( lpipeline "pipeline" )
else()
  message( STATUS "Developer local build" )
  set( lpipeline "local" )
endif()

if(${GIT_BRANCH} STREQUAL "main" )
  if( ${AZURE_PIPELINE} )
    set( lverpatch "nightly" )
  else()
    set( lverpatch "main" )
  endif()
  set( lverrel "999" )
  set( fverpatch ${lverpatch} )
else()
  string( FIND ${GIT_BRANCH} "release/" RLOC )
  if( ${RLOC} EQUAL 0 )
    message( STATUS "Configuring a Release build from '${GIT_BRANCH}'" )
    string( SUBSTRING ${GIT_BRANCH} 11 100 RV ) # that's release slash 1.7.
    string( FIND ${RV} "." DLOC )
    if( NOT ( DLOC EQUAL -1 ) )
      math( EXPR DLP1 "${DLOC} + 1" )
      string( SUBSTRING ${RV} ${DLP1} 100 LRV ) # skip that first dots
      set( lverrel ${LRV} )
    else()
      set( lverrel "99" )
    endif()
    set( lverpatch "stable-${lverrel}" )
    set( fverpatch "${lverrel}" )
  else()
    set( lverpatch ${GIT_BRANCH} )
    set( fverpatch ${lverpatch} )
    set( lverrel "1000" )
  endif()
endif()

set( CONDUIT_FULL_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${fverpatch}.${GIT_COMMIT_HASH}" )
set( CONDUIT_MAJOR_VERSION "${PROJECT_VERSION_MAJOR}" )
set( CONDUIT_SUB_VERSION "${PROJECT_VERSION_MINOR}" )
set( CONDUIT_RELEASE_VERSION "${lverpatch}" )
set( CONDUIT_RELEASE_NUMBER "${lverrel}" )
set( CONDUIT_BUILD_HASH "${GIT_COMMIT_HASH}" )
set( CONDUIT_BUILD_LOCATION "${lpipeline}" )

string( TIMESTAMP CONDUIT_BUILD_DATE "%Y-%m-%d" )
string( TIMESTAMP CONDUIT_BUILD_YEAR "%Y" )
string( TIMESTAMP CONDUIT_BUILD_TIME "%H:%M:%S" )

message( STATUS "Using CONDUIT_VERSION=${CONDUIT_FULL_VERSION}" )

message( STATUS "Configuring ${CONDUITBLD}/geninclude/version.cpp" )
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cpp.in
  ${CONDUITBLD}/geninclude/version.cpp )
file(WRITE ${CONDUITBLD}/geninclude/githash.txt ${GIT_COMMIT_HASH})
