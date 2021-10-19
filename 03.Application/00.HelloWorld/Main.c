// initialized global variable
int g_iB = 3;

// test function
void A(void)
{
	// store 4 to global variable
	g_iB = 4;
}

// entry point
void _START(void)
{
	A();

	return;
}
