noinst_LIBRARIES = libmuleunit.a
libmuleunit_a_SOURCES = \
	test.cpp \
	testresult.cpp \
	main.cpp \
	testcase.cpp \
	testregistry.cpp \
	$(top_srcdir)/src/libs/common/MuleDebug.cpp \
	$(top_srcdir)/src/libs/common/StringFunctions.cpp

libmuleunit_a_CXXFLAGS = $(WXBASE_CXXFLAGS) -I$(top_srcdir)/src/libs/common/ -DMULEUNIT 
