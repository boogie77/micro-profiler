#include <frontend/symbol_resolver.h>

#include "Helpers.h"

using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		typedef void (*get_function_addresses_1_t)(const void *&f1, const void *&f2);
		typedef void (*get_function_addresses_2_t)(const void *&f1, const void *&f2, const void *&f3);
		typedef void (*get_function_addresses_3_t)(const void *&f);

		[TestClass]
		[DeploymentItem("symbol_container_1.dll"), DeploymentItem("symbol_container_1.pdb")]
		[DeploymentItem("symbol_container_2.dll"), DeploymentItem("symbol_container_2.pdb")]
		[DeploymentItem("symbol_container_3_nosymbols.dll")]
		public ref class SymbolResolverTests
		{
		public:
			[TestMethod]
			void ResolverCreationReturnsNonNullObject()
			{
				// INIT / ACT / ASSERT
				Assert::IsTrue(!!symbol_resolver::create());
			}


			[TestMethod]
			void LoadImageFailsWhenInvalidModuleSpecified()
			{
				// INIT
				shared_ptr<symbol_resolver> r(symbol_resolver::create());

				// ACT / ASSERT
				ASSERT_THROWS(r->add_image(L"", reinterpret_cast<const void *>(0x12345)), invalid_argument);
				ASSERT_THROWS(r->add_image(L"missingABCDEFG.dll", reinterpret_cast<const void *>(0x23451)), invalid_argument);
			}


			[TestMethod]
			void CreateResolverForValidImage()
			{
				// INIT
				image img1(_T("micro-profiler.tests.dll"));
				image img2(_T("symbol_container_1.dll"));
				shared_ptr<symbol_resolver> r(symbol_resolver::create());

				// ACT / ASSERT (must not throw)
				r->add_image(img1.absolute_path(), img1.load_address());
				r->add_image(img1.absolute_path(),
					reinterpret_cast<const void *>(reinterpret_cast<uintptr_t>(img1.load_address()) + 0x100000));
				r->add_image(img2.absolute_path(), img2.load_address());
			}


			[TestMethod]
			void ReturnNamesOfLocalFunctions1()
			{
				// INIT
				image img(_T("symbol_container_1.dll"));
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_1_t getter_1 =
					reinterpret_cast<get_function_addresses_1_t>(img.get_symbol_address("get_function_addresses_1"));
				const void *f1 = 0, *f2 = 0;

				r->add_image(img.absolute_path(), img.load_address());
				getter_1(f1, f2);

				// ACT
				wstring name1 = r->symbol_name_by_va(f1);
				wstring name2 = r->symbol_name_by_va(f2);

				// ASSERT
				Assert::IsTrue(name1 == L"very_simple_global_function");
				Assert::IsTrue(name2 == L"a_tiny_namespace::function_that_hides_under_a_namespace");
			}


			[TestMethod]
			void ReturnNamesOfLocalFunctions2()
			{
				// INIT
				image img(_T("symbol_container_2.dll"));
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_2_t getter_2 =
					reinterpret_cast<get_function_addresses_2_t>(img.get_symbol_address("get_function_addresses_2"));
				const void *f1 = 0, *f2 = 0, *f3 = 0;

				r->add_image(img.absolute_path(), img.load_address());
				getter_2(f1, f2, f3);

				// ACT
				wstring name1 = r->symbol_name_by_va(f1);
				wstring name2 = r->symbol_name_by_va(f2);
				wstring name3 = r->symbol_name_by_va(f3);

				// ASSERT
				Assert::IsTrue(name1 == L"vale_of_mean_creatures::this_one_for_the_birds");
				Assert::IsTrue(name2 == L"vale_of_mean_creatures::this_one_for_the_whales");
				Assert::IsTrue(name3 == L"vale_of_mean_creatures::the_abyss::bubble_sort");
			}


			[TestMethod]
			void RespectLoadAddress()
			{
				// INIT
				image img(_T("symbol_container_2.dll"));
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_2_t getter_2 =
					reinterpret_cast<get_function_addresses_2_t>(img.get_symbol_address("get_function_addresses_2"));
				const void *f1 = 0, *f2 = 0, *f3 = 0;

				r->add_image(img.absolute_path(),
					reinterpret_cast<const void *>(reinterpret_cast<uintptr_t>(img.load_address()) + 113));
				getter_2(f1, f2, f3);

				*reinterpret_cast<const char **>(&f1) += 113;
				*reinterpret_cast<const char **>(&f2) += 113;
				*reinterpret_cast<const char **>(&f3) += 113;

				// ACT
				wstring name1 = r->symbol_name_by_va(f1);
				wstring name2 = r->symbol_name_by_va(f2);
				wstring name3 = r->symbol_name_by_va(f3);

				// ASSERT
				Assert::IsTrue(name1 == L"vale_of_mean_creatures::this_one_for_the_birds");
				Assert::IsTrue(name2 == L"vale_of_mean_creatures::this_one_for_the_whales");
				Assert::IsTrue(name3 == L"vale_of_mean_creatures::the_abyss::bubble_sort");
			}


			[TestMethod]
			void LoadModuleWithNoSymbols()
			{
				// INIT
				image img(_T("symbol_container_3_nosymbols.dll"));
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_3_t getter_3 =
					reinterpret_cast<get_function_addresses_3_t>(img.get_symbol_address("get_function_addresses_3"));
				const void *f = 0;

				r->add_image(img.absolute_path(), img.load_address());
				getter_3(f);

				// ACT
				wstring name1 = r->symbol_name_by_va(getter_3);
				wstring name2 = r->symbol_name_by_va(f);

				// ASSERT
				Assert::IsTrue(name1 == L"get_function_addresses_3"); // exported function will have a name
				Assert::IsTrue(name2 == L"");
			}


			[TestMethod]
			void ConstantReferenceFromResolverIsTheSame()
			{
				// INIT
				image img(_T("symbol_container_2.dll"));
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_2_t getter_2 =
					reinterpret_cast<get_function_addresses_2_t>(img.get_symbol_address("get_function_addresses_2"));
				const void *f1 = 0, *f2 = 0, *f3 = 0;

				r->add_image(img.absolute_path(), img.load_address());
				getter_2(f1, f2, f3);

				// ACT
				const wstring *name1_1 = &r->symbol_name_by_va(f1);
				const wstring *name2_1 = &r->symbol_name_by_va(f2);
				const wstring *name3_1 = &r->symbol_name_by_va(f3);

				const wstring *name2_2 = &r->symbol_name_by_va(f2);
				const wstring *name1_2 = &r->symbol_name_by_va(f1);
				const wstring *name3_2 = &r->symbol_name_by_va(f3);

				// ASSERT
				Assert::IsTrue(name1_1 == name1_2);
				Assert::IsTrue(name2_1 == name2_2);
				Assert::IsTrue(name3_1 == name3_2);
			}


			[TestMethod]
			void LoadSymbolsForSecondModule()
			{
				// INIT
				image img1(_T("symbol_container_1.dll")), img2(_T("symbol_container_2.dll"));
				shared_ptr<symbol_resolver> r(symbol_resolver::create());
				get_function_addresses_1_t getter_1 =
					reinterpret_cast<get_function_addresses_1_t>(img1.get_symbol_address("get_function_addresses_1"));
				get_function_addresses_2_t getter_2 =
					reinterpret_cast<get_function_addresses_2_t>(img2.get_symbol_address("get_function_addresses_2"));
				const void  *f1_1 = 0, *f2_1 = 0, *f1_2 = 0, *f2_2 = 0, *f3_2 = 0;

				getter_1(f1_1, f2_1);
				getter_2(f1_2, f2_2, f3_2);

				// ACT
				r->add_image(img1.absolute_path(), img1.load_address());
				r->add_image(img2.absolute_path(), img2.load_address());

				// ACT / ASSERT
				Assert::IsTrue(r->symbol_name_by_va(f1_2) == L"vale_of_mean_creatures::this_one_for_the_birds");
				Assert::IsTrue(r->symbol_name_by_va(f2_2) == L"vale_of_mean_creatures::this_one_for_the_whales");
				Assert::IsTrue(r->symbol_name_by_va(f3_2) == L"vale_of_mean_creatures::the_abyss::bubble_sort");
				Assert::IsTrue(r->symbol_name_by_va(f1_1) == L"very_simple_global_function");
				Assert::IsTrue(r->symbol_name_by_va(f2_1) == L"a_tiny_namespace::function_that_hides_under_a_namespace");
			}
		};
	}
}
