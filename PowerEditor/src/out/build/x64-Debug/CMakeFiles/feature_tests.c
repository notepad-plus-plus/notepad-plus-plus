
  const char features[] = {"\n"
"C_FEATURE:"
#if _MSC_VER >= 1600
"1"
#else
"0"
#endif
"c_function_prototypes\n"
"C_FEATURE:"
#if _MSC_VER >= 1600
"1"
#else
"0"
#endif
"c_variadic_macros\n"

};

int main(int argc, char** argv) { (void)argv; return features[argc]; }
