#include <collector/analyzer.h>

#include "Helpers.h"

#include <map>

using namespace std;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			bool has_empty_statistics(const analyzer &a)
			{
				for (analyzer::const_iterator i = a.begin(); i != a.end(); ++i)
					if (i->second.times_called || i->second.inclusive_time || i->second.exclusive_time || i->second.max_reentrance || i->second.max_call_time)
						return false;
				return true;
			}
		}

		[TestClass]
		public ref class AnalyzerTests
		{
		public: 
			[TestMethod]
			void NewAnalyzerHasNoFunctionRecords()
			{
				// INIT / ACT
				analyzer a;

				// ACT / ASSERT
				Assert::IsTrue(a.begin() == a.end());
			}


			[TestMethod]
			void EnteringOnlyToFunctionsLeavesOnlyEmptyStatTraces()
			{
				// INIT
				analyzer a;
				calls_collector_i::acceptor &as_acceptor(a);
				call_record trace[] = {
					{	12300, (void *)1234	},
					{	12305, (void *)2234	},
				};

				// ACT
				as_acceptor.accept_calls(1, trace, array_size(trace));
				as_acceptor.accept_calls(2, trace, array_size(trace));

				// ASSERT
				Assert::IsTrue(2 == distance(a.begin(), a.end()));
				Assert::IsTrue(has_empty_statistics(a));
			}


			[TestMethod]
			void EvaluateSeveralFunctionDurations()
			{
				// INIT
				analyzer a;
				call_record trace[] = {
					{	12300, (void *)1234	},
					{	12305, (void *)0	},
					{	12310, (void *)2234	},
					{	12317, (void *)0	},
					{	12320, (void *)2234	},
					{	12322, (void *)12234	},
					{	12325, (void *)0	},
					{	12327, (void *)0	},
				};

				// ACT
				a.accept_calls(1, trace, array_size(trace));

				// ASSERT
				map<const void *, function_statistics> m(a.begin(), a.end());	// use map to ensure proper sorting

				Assert::IsTrue(3 == m.size());

				map<const void *, function_statistics>::const_iterator i1(m.begin()), i2(m.begin()), i3(m.begin());

				++i2, ++++i3;

				Assert::IsTrue(1 == i1->second.times_called);
				Assert::IsTrue(5 == i1->second.inclusive_time);
				Assert::IsTrue(5 == i1->second.exclusive_time);

				Assert::IsTrue(2 == i2->second.times_called);
				Assert::IsTrue(14 == i2->second.inclusive_time);
				Assert::IsTrue(11 == i2->second.exclusive_time);

				Assert::IsTrue(1 == i3->second.times_called);
				Assert::IsTrue(3 == i3->second.inclusive_time);
				Assert::IsTrue(3 == i3->second.exclusive_time);
			}


			[TestMethod]
			void AnalyzerCollectsDetailedStatistics()
			{
				// INIT
				analyzer a;
				call_record trace[] = {
					{	1, (void *)1	},
						{	2, (void *)11	},
						{	3, (void *)0	},
					{	4, (void *)0	},
					{	5, (void *)2	},
						{	10, (void *)21	},
						{	11, (void *)0	},
						{	13, (void *)22	},
						{	17, (void *)0	},
					{	23, (void *)0	},
				};

				// ACT
				a.accept_calls(1, trace, array_size(trace));

				// ASSERT
				map<const void *, function_statistics_detailed> m(a.begin(), a.end());	// use map to ensure proper sorting

				Assert::IsTrue(5 == m.size());

				Assert::IsTrue(1 == m[(void *)1].callees.size());
				Assert::IsTrue(2 == m[(void *)2].callees.size());
				Assert::IsTrue(0 == m[(void *)11].callees.size());
				Assert::IsTrue(0 == m[(void *)21].callees.size());
				Assert::IsTrue(0 == m[(void *)22].callees.size());
			}


			[TestMethod]
			void ProfilerLatencyIsTakenIntoAccount()
			{
				// INIT
				analyzer a(1);
				call_record trace[] = {
					{	12300, (void *)1234	},
					{	12305, (void *)0	},
					{	12310, (void *)2234	},
					{	12317, (void *)0	},
					{	12320, (void *)2234	},
					{	12322, (void *)12234	},
					{	12325, (void *)0	},
					{	12327, (void *)0	},
				};

				// ACT
				a.accept_calls(1, trace, array_size(trace));

				// ASSERT
				map<const void *, function_statistics> m(a.begin(), a.end());	// use map to ensure proper sorting

				Assert::IsTrue(3 == m.size());

				map<const void *, function_statistics>::const_iterator i1(m.begin()), i2(m.begin()), i3(m.begin());

				++i2, ++++i3;

				Assert::IsTrue(1 == i1->second.times_called);
				Assert::IsTrue(4 == i1->second.inclusive_time);
				Assert::IsTrue(4 == i1->second.exclusive_time);

				Assert::IsTrue(2 == i2->second.times_called);
				Assert::IsTrue(12 == i2->second.inclusive_time);
				Assert::IsTrue(8 == i2->second.exclusive_time);

				Assert::IsTrue(1 == i3->second.times_called);
				Assert::IsTrue(2 == i3->second.inclusive_time);
				Assert::IsTrue(2 == i3->second.exclusive_time);
			}


			[TestMethod]
			void DifferentShadowStacksAreMaintainedForEachThread()
			{
				// INIT
				analyzer a;
				map<const void *, function_statistics> m;
				call_record trace1[] = {	{	12300, (void *)1234	},	};
				call_record trace2[] = {	{	12313, (void *)1234	},	};
				call_record trace3[] = {	{	12307, (void *)0	},	};
				call_record trace4[] = {
					{	12319, (void *)0	},
					{	12323, (void *)1234	},
				};

				// ACT
				a.accept_calls(1, trace1, array_size(trace1));
				a.accept_calls(2, trace2, array_size(trace2));

				// ASSERT
				Assert::IsTrue(has_empty_statistics(a));

				// ACT
				a.accept_calls(1, trace3, array_size(trace3));

				// ASSERT
				Assert::IsTrue(1 == distance(a.begin(), a.end()));
				Assert::IsTrue((void *)1234 == a.begin()->first);
				Assert::IsTrue(1 == a.begin()->second.times_called);
				Assert::IsTrue(7 == a.begin()->second.inclusive_time);
				Assert::IsTrue(7 == a.begin()->second.exclusive_time);

				// ACT
				a.accept_calls(2, trace4, array_size(trace4));

				// ASSERT
				Assert::IsTrue(1 == distance(a.begin(), a.end()));
				Assert::IsTrue((void *)1234 == a.begin()->first);
				Assert::IsTrue(2 == a.begin()->second.times_called);
				Assert::IsTrue(13 == a.begin()->second.inclusive_time);
				Assert::IsTrue(13 == a.begin()->second.exclusive_time);
			}


			[TestMethod]
			void ClearingRemovesPreviousStatButLeavesStackStates()
			{
				// INIT
				analyzer a;
				map<const void *, function_statistics> m;
				call_record trace1[] = {
					{	12319, (void *)1234	},
					{	12324, (void *)0	},
					{	12324, (void *)2234	},
					{	12326, (void *)0	},
					{	12330, (void *)2234	},
				};
				call_record trace2[] = {	{	12350, (void *)0	},	};

				a.accept_calls(2, trace1, array_size(trace1));

				// ACT
				a.clear();
				a.accept_calls(2, trace2, array_size(trace2));

				// ASSERT
				Assert::IsTrue(1 == distance(a.begin(), a.end()));
				Assert::IsTrue((void *)2234 == a.begin()->first);
				Assert::IsTrue(1 == a.begin()->second.times_called);
				Assert::IsTrue(20 == a.begin()->second.inclusive_time);
				Assert::IsTrue(20 == a.begin()->second.exclusive_time);
			}
		};
	}
}
