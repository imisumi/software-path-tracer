#include "App.h"

int main()
{
	App app;
	app.run();
	return 0;
}

// #include <iostream>
// #include <vector>

// // Generates code for every target that this compiler can support.
// #undef HWY_TARGET_INCLUDE
// #define HWY_TARGET_INCLUDE "main.cpp" // this file
// #include <hwy/foreach_target.h>		  // must come before highway.h
// #include <hwy/highway.h>

// namespace project
// {
// 	namespace HWY_NAMESPACE
// 	{ // required: unique per target

// 		// Can skip hn:: prefixes if already inside hwy::HWY_NAMESPACE.
// 		namespace hn = hwy::HWY_NAMESPACE;

// 		using T = float;

// 		// Alternative to per-function HWY_ATTR: see HWY_BEFORE_NAMESPACE
// 		HWY_ATTR void MulAddLoop(const T *HWY_RESTRICT mul_array,
// 								 const T *HWY_RESTRICT add_array,
// 								 const size_t size, T *HWY_RESTRICT x_array)
// 		{
// 			const hn::ScalableTag<T> d;
// 			const size_t lanes = hn::Lanes(d);

// 			std::cout << "Using SIMD with " << lanes << " lanes per vector\n";
// 			std::cout << "Target: " << hwy::TargetName(HWY_TARGET) << "\n";

// 			for (size_t i = 0; i < size; i += lanes)
// 			{
// 				const auto mul = hn::Load(d, mul_array + i);
// 				const auto add = hn::Load(d, add_array + i);
// 				auto x = hn::Load(d, x_array + i);
// 				x = hn::MulAdd(mul, x, add); // x = mul * x + add
// 				hn::Store(x, d, x_array + i);
// 			}
// 		}

// 	} // namespace HWY_NAMESPACE
// } // namespace project

// // The table of pointers to the various implementations in HWY_NAMESPACE must
// // be compiled only once (foreach_target #includes this file multiple times).
// // HWY_ONCE is true for only one of these 'compilation passes'.
// #if HWY_ONCE

// namespace project
// {

// 	// This macro declares a static array used for dynamic dispatch.
// 	HWY_EXPORT(MulAddLoop);

// 	void CallMulAddLoop(const float *HWY_RESTRICT mul_array,
// 						const float *HWY_RESTRICT add_array,
// 						const size_t size, float *HWY_RESTRICT x_array)
// 	{
// 		// This must reside outside of HWY_NAMESPACE because it references (calls the
// 		// appropriate one from) the per-target implementations there.
// 		// For static dispatch, use HWY_STATIC_DISPATCH.
// 		return HWY_DYNAMIC_DISPATCH(MulAddLoop)(mul_array, add_array, size, x_array);
// 	}

// } // namespace project

// #endif // HWY_ONCE

// int main()
// {
// 	// Create test data
// 	const size_t size = 8;
// 	std::vector<float> mul_array = {2.0f, 3.0f, 4.0f, 5.0f, 1.5f, 2.5f, 3.5f, 4.5f};
// 	std::vector<float> add_array = {1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.5f};
// 	std::vector<float> x_array = {10.0f, 20.0f, 30.0f, 40.0f, 100.0f, 200.0f, 300.0f, 400.0f};

// 	std::cout << "Initial values:\n";
// 	std::cout << "mul: ";
// 	for (const auto &val : mul_array)
// 		std::cout << val << " ";
// 	std::cout << "\nadd: ";
// 	for (const auto &val : add_array)
// 		std::cout << val << " ";
// 	std::cout << "\nx:   ";
// 	for (const auto &val : x_array)
// 		std::cout << val << " ";
// 	std::cout << "\n\n";

// 	// Perform mul-add operation: x = mul * x + add
// 	project::CallMulAddLoop(mul_array.data(), add_array.data(), size, x_array.data());

// 	std::cout << "After MulAdd (x = mul * x + add):\n";
// 	std::cout << "x:   ";
// 	for (const auto &val : x_array)
// 		std::cout << val << " ";
// 	std::cout << "\n\n";

// 	// Verify manually for first element: 2.0 * 10.0 + 1.0 = 21.0
// 	std::cout << "Manual verification for first element:\n";
// 	std::cout << "2.0 * 10.0 + 1.0 = " << (2.0f * 10.0f + 1.0f) << "\n";
// 	std::cout << "Result: " << x_array[0] << "\n";

// 	return 0;
// }