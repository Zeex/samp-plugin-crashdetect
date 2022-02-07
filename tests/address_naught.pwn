native printf(const string[], {Float, _}:...);

#include <crashdetect>

// Not located at address 0, even though it is first.
new var = 6;

// The anonymous automata is always address 0.
Func() <state_a>
{
	printf("state_a");
}

Func() <state_b>
{
	printf("state_b");
}

Func() <>
{
	printf("fallback");
}

main()
{
	printf("Address 0 write detection.");
	printf("Available: %d", HasCrashDetectAddr0());
	DisableCrashDetectAddr0();
	printf("Enabled: %d", IsCrashDetectAddr0Enabled());
	EnableCrashDetectAddr0();
	printf("Enabled: %d", IsCrashDetectAddr0Enabled());

	// Test variable writing (shouldn't trigger).
	printf("Var: %d", var);
	DisableCrashDetectAddr0();
	var = 7;
	printf("Var: %d", var);
	EnableCrashDetectAddr0();
	var = 8;
	printf("Var: %d", var);

	// Test state writing (should trigger).
	Func();
	DisableCrashDetectAddr0();
	state state_a;
	Func();
	EnableCrashDetectAddr0();
	state state_b;
	Func();
}

