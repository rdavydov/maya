cl /TP -c /Ox /Ot /Oy /GF /GS- /MD /nologo /W3 /Zc:forScope /EHsc /Ob2 -DQMC -DMI_MODULE= -DMI_PRODUCT_RAY -DWIN_NT -DEVIL_ENDIAN -D_WIN32_WINNT=0x0400 -DNV_CG -D_SECURE_SCL=0 -DHYPERTHREAD -DX86 -I. -I.  ./dgsutil.cpp
cl /TP -c /Ox /Ot /Oy /GF /GS- /MD /nologo /W3 /Zc:forScope /EHsc /Ob2 -DQMC -DMI_MODULE= -DMI_PRODUCT_RAY -DWIN_NT -DEVIL_ENDIAN -D_WIN32_WINNT=0x0400 -DNV_CG -D_SECURE_SCL=0 -DHYPERTHREAD -DX86 -I. -I.  ./dgsshade.cpp
cl /TP -c /Ox /Ot /Oy /GF /GS- /MD /nologo /W3 /Zc:forScope /EHsc /Ob2 -DQMC -DMI_MODULE= -DMI_PRODUCT_RAY -DWIN_NT -DEVIL_ENDIAN -D_WIN32_WINNT=0x0400 -DNV_CG -D_SECURE_SCL=0 -DHYPERTHREAD -DX86 -I. -I.  ./dielecshade.cpp
cl /TP -c /Ox /Ot /Oy /GF /GS- /MD /nologo /W3 /Zc:forScope /EHsc /Ob2 -DQMC -DMI_MODULE= -DMI_PRODUCT_RAY -DWIN_NT -DEVIL_ENDIAN -D_WIN32_WINNT=0x0400 -DNV_CG -D_SECURE_SCL=0 -DHYPERTHREAD -DX86 -I. -I.  ./partishade.cpp
cl /TP -c /Ox /Ot /Oy /GF /GS- /MD /nologo /W3 /Zc:forScope /EHsc /Ob2 -DQMC -DMI_MODULE= -DMI_PRODUCT_RAY -DWIN_NT -DEVIL_ENDIAN -D_WIN32_WINNT=0x0400 -DNV_CG -D_SECURE_SCL=0 -DHYPERTHREAD -DX86 -I. -I.  ./pathshade.cpp
cl /TP -c /Ox /Ot /Oy /GF /GS- /MD /nologo /W3 /Zc:forScope /EHsc /Ob2 -DQMC -DMI_MODULE= -DMI_PRODUCT_RAY -DWIN_NT -DEVIL_ENDIAN -D_WIN32_WINNT=0x0400 -DNV_CG -D_SECURE_SCL=0 -DHYPERTHREAD -DX86 -I. -I.  ./physlight.cpp
cl /TP -c /Ox /Ot /Oy /GF /GS- /MD /nologo /W3 /Zc:forScope /EHsc /Ob2 -DQMC -DMI_MODULE= -DMI_PRODUCT_RAY -DWIN_NT -DEVIL_ENDIAN -D_WIN32_WINNT=0x0400 -DNV_CG -D_SECURE_SCL=0 -DHYPERTHREAD -DX86 -I. -I.  ./physlens.cpp
link /delayload:opengl32.dll /nologo /nodefaultlib:LIBC.LIB /MAP:mapfile /OPT:NOREF /INCREMENTAL:NO /LIBPATH:..\..\lib /STACK:0x200000,0x1000 ws2_32.lib user32.lib mpr.lib opengl32.lib gdi32.lib delayimp.lib /DLL /OUT:physics.dll dgsutil.obj dgsshade.obj dielecshade.obj partishade.obj pathshade.obj physlight.obj physlens.obj ../../nt-x86-vc8/lib/shader.lib
mt.exe -nologo -manifest physics.dll.manifest -outputresource:physics.dll;2
