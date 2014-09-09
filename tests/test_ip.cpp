// Petter Strandmark 2013–2014.

#include <catch.hpp>

#include <easy-ip.h>

using namespace std;

TEST_CASE("solve_boolean_infeasible")
{
	IP ip;
	auto x = ip.add_variable();
	auto y = ip.add_variable();
	ip.add_constraint(x + y >= 3);
	CHECK( ! ip.solve() );
}

TEST_CASE("solve_boolean_feasible")
{
	IP ip;
	auto x = ip.add_variable();
	auto y = ip.add_variable();
	ip.add_constraint(x + y >= 2);
	CHECK( ip.solve() );
}

TEST_CASE("solve_real_unbounded")
{
	IP ip;
	auto x = ip.add_variable(IP::Real, -1.0);
	auto y = ip.add_variable(IP::Real, -1.0);
	ip.add_constraint(x + y >= 3);
	CHECK( ! ip.solve() );
}

TEST_CASE("solve_real_feasible")
{
	IP ip;
	auto x = ip.add_variable(IP::Real, -1.0);
	auto y = ip.add_variable(IP::Real, -1.0);
	ip.set_bounds(-100, x, 100);
	ip.set_bounds(-100, y, 100);
	ip.add_constraint(x + y <= 3);
	CHECK( ip.solve() );
}

TEST_CASE("different_solvers")
{
	IP ip1, ip2;
	auto x = ip1.add_boolean();
	auto y = ip2.add_boolean();
	CHECK_THROWS(x + y);
	CHECK_THROWS(x - y);
	CHECK_THROWS(x || y);
	CHECK_THROWS(ip1.add_objective(y));
	CHECK_THROWS(ip2.set_bounds(0, x, 2));
	CHECK_THROWS(x == y);
	CHECK_THROWS(x >= y);
	CHECK_THROWS(x <= y);
	CHECK_THROWS(ip1.add_constraint(2.0 * y >= 8));
	CHECK_THROWS(ip2.add_constraint(x + x >= 8));
	ip1.solve();
	ip2.solve();
	CHECK_THROWS(ip1.get_solution(y));
	CHECK_THROWS(ip1.get_solution(!y));
}

auto create_soduku_IP(IP& ip, int n = 3)
	-> decltype(ip.add_boolean_cube(9, 9, 9))
{
	auto P = ip.add_boolean_cube(n*n, n*n, n*n);

	// Exactly one indicator equal to 1.
	for (int i = 0; i < n*n; ++i) {
		for (int j = 0; j < n*n; ++j) {
			Sum k_sum;
			for (int k = 0; k < n*n; ++k) {
				k_sum += P[i][j][k];
			}
			ip.add_constraint(k_sum == 1);

			// Advanced tip: One can use std::move to avoid a copy
			// here:
			//        ip.add_constraint(move(k_sum) == 1);
		}
	}

	// All rows have every number.
	for (int i = 0; i < n*n; ++i) {
		for (int k = 0; k < n*n; ++k) {
			Sum row_k_sum;
			for (int j = 0; j < n*n; ++j) {
				row_k_sum += P[i][j][k];
			}
			ip.add_constraint(row_k_sum == 1);
		}
	}

	// All columns have every number.
	for (int j = 0; j < n*n; ++j) {
		for (int k = 0; k < n*n; ++k) {
			Sum col_k_sum;
			for (int i = 0; i < n*n; ++i) {
				col_k_sum += P[i][j][k];
			}
			ip.add_constraint(col_k_sum == 1);
		}
	}

	// The n*n subsquares have every number.
	for (int i1 = 0; i1 < n; ++i1) {
		for (int j1 = 0; j1 < n; ++j1) {
			for (int k = 0; k < n*n; ++k) {
				Sum square_k_sum;
				for (int i2 = 0; i2 < n; ++i2) {
					for (int j2 = 0; j2 < n; ++j2) {
						square_k_sum += P[n*i1 + i2][n*j1 + j2][k];
					}
				}
				ip.add_constraint(square_k_sum == 1);
			}
		}
	}
	return P;
}

TEST_CASE("sudoku")
{
	using namespace std;

	IP ip;
	int n = 3;
	auto P = create_soduku_IP(ip);

	REQUIRE(ip.solve());

	vector<vector<int>> solution(n*n);

	cout << endl;
	for (int i = 0; i < n*n; ++i) {
		for (int j = 0; j < n*n; ++j) {
			solution[i].emplace_back();

			for (int k = 0; k < n*n; ++k) {
				if (P[i][j][k].value()) {
					solution[i][j] = k + 1;
				}
			}
		}
	}

	for (int i = 0; i < n*n; ++i) {
		int row_sum = 0;
		int col_sum = 0;
		for (int j = 0; j < n*n; ++j) {
			row_sum += solution[i][j];
			col_sum += solution[j][i];
		}
		CHECK(row_sum == 45);
		CHECK(col_sum == 45);
	}
}

TEST_CASE("move_constructor")
{
	IP ip;
	auto x = ip.add_variable(IP::Real, -1.0);
	auto y = ip.add_variable(IP::Real, -1.0);
	ip.set_bounds(-100, x, 100);
	ip.set_bounds(-100, y, 100);
	ip.add_constraint(x + y <= 3);

	IP ip2 = std::move(ip);

	CHECK( ip2.solve() );
}

TEST_CASE("add_variable_as_booleans")
{
	IP ip;
	auto x = ip.add_variable_as_booleans(-3, 3);
	auto y = ip.add_variable_as_booleans({-3, -2, 3, 2, 1});
	ip.add_objective(x - y);
	CHECK(ip.solve());

	CHECK(x.value() == -3);
	CHECK(y.value() == 3);
}

#ifdef HAS_MINISAT
TEST_CASE("minisat")
{
	{
		IP ip;
		ip.set_external_solver(IP::Minisat);
		auto x = ip.add_boolean();
		auto y = ip.add_boolean();
		ip.add_constraint(x + y == 1);
		REQUIRE(ip.solve());
		CHECK((x.value() + y.value() == 1));

		auto z = ip.add_boolean();
		ip.add_constraint(x + z == 1);
		ip.add_constraint(y + z == 1);
		CHECK( ! ip.solve());
		ip.add_constraint(x + y + z == 1);
		CHECK( ! ip.solve());
	}

	{
		IP ip;
		ip.set_external_solver(IP::Minisat);
		auto x = ip.add_boolean();
		ip.add_objective(-x);
		CHECK_THROWS(ip.solve());	
	}

	{
		IP ip;
		ip.set_external_solver(IP::Minisat);
		auto x = ip.add_boolean();
		auto y = ip.add_boolean();
		ip.add_objective(2*x + y);
		ip.add_constraint(x + y <= 1);
		ip.allow_ignoring_cost_function();
		CHECK(ip.solve());	
	}

	{
		IP ip;
		ip.set_external_solver(IP::Minisat);
		auto x = ip.add_boolean();
		auto y = ip.add_boolean();
		ip.add_constraint(x + y <= 1);
		CHECK(ip.solve());
		int num_solutions = 0;
		do {
			num_solutions++;
		} while (ip.next_solution());
		CHECK(num_solutions == 3);
	}

	{
		IP ip;
		ip.set_external_solver(IP::Minisat);
		auto x = ip.add_boolean();
		auto y = ip.add_boolean();
		auto z = ip.add_boolean();
		auto w = ip.add_boolean();
		auto u = ip.add_boolean();
		auto v = ip.add_boolean();
		ip.add_constraint(x + y + z + w + u + v <= 5);
		ip.add_constraint(x + y + z + w + u + v >= 2);
		CHECK(ip.solve());
		int num_solutions = 0;
		do {
			num_solutions++;
		} while (ip.next_solution());
		CHECK(num_solutions == 56);
	}

	{
		IP ip;
		ip.set_external_solver(IP::Minisat);
		auto x = ip.add_boolean();
		auto y = ip.add_boolean();
		ip.add_constraint(x + y == 2);
		CHECK(ip.solve());
		int num_solutions = 0;
		do {
			num_solutions++;
		} while (ip.next_solution());
		CHECK(num_solutions == 1);
	}

	{
		IP ip;
		ip.set_external_solver(IP::Minisat);
		auto x = ip.add_boolean();
		auto y = ip.add_boolean();
		ip.add_constraint(x + 2*y == 1);
		CHECK_THROWS(ip.solve());
	}

	{
		IP ip;
		ip.set_external_solver(IP::Minisat);
		auto x = ip.add_boolean();
		auto y = ip.add_boolean();
		ip.add_constraint(x - y == 1);
		CHECK(ip.solve());
		int num_solutions = 0;
		do {
			num_solutions++;
		} while (ip.next_solution());
		CHECK(num_solutions == 1);
	}

	{
		IP ip;
		ip.set_external_solver(IP::Minisat);
		auto x = ip.add_boolean();
		auto y = ip.add_boolean();
		auto z = ip.add_boolean();
		auto w = ip.add_boolean();
		ip.add_constraint(x + y - z - w == 0);
		CHECK(ip.solve());
		int num_solutions = 0;
		do {
			num_solutions++;
		} while (ip.next_solution());
		CHECK(num_solutions == 6);
	}

	{
		IP ip;
		ip.set_external_solver(IP::Minisat);
		auto x = ip.add_boolean();
		auto y = ip.add_boolean();
		auto z = ip.add_boolean();
		ip.add_constraint(x + y + z == 1);
		REQUIRE(ip.solve());
		int num_solutions = 0;
		do {
			num_solutions++;
		} while (ip.next_solution());
		CHECK(num_solutions == 3);
	}

	{
		IP ip;
		int n = 3;
		auto P = create_soduku_IP(ip, n);

		ip.set_external_solver(IP::Minisat);
		REQUIRE(ip.solve());
		 
		vector<vector<int>> solution(n*n);

		cout << endl;
		for (int i = 0; i < n*n; ++i) {
			for (int j = 0; j < n*n; ++j) {
				solution[i].emplace_back();

				for (int k = 0; k < n*n; ++k) {
					if (P[i][j][k].value()) {
						solution[i][j] = k + 1;
					}
				}
			}
		}

		for (int i = 0; i < n*n; ++i) {
			int row_sum = 0;
			int col_sum = 0;
			for (int j = 0; j < n*n; ++j) {
				row_sum += solution[i][j];
				col_sum += solution[j][i];
			}
			CHECK(row_sum == 45);
			CHECK(col_sum == 45);
		}
	}
}

TEST_CASE("minisat-large")
{
	IP ip;
	ip.set_external_solver(IP::Minisat);
	ip.allow_ignoring_cost_function();
	Sum sum = 0;
	for (int i = 0; i < 100; ++i) {
		auto x = ip.add_boolean(1);
		sum += x;
	}
	ip.add_constraint(sum == 50);
	CHECK(ip.solve());
	CHECK(sum.value() == 50);
}

TEST_CASE("sat-objective")
{
	IP ip;
	ip.set_external_solver(IP::Minisat);
	auto x = ip.add_boolean();
	auto y = ip.add_boolean();
	auto z = ip.add_boolean();
	auto w = ip.add_boolean();
	ip.add_objective(4*x + y + 2*z + w);
	ip.add_constraint(x + y + z + w == 2);
	CHECK(ip.solve());
	CHECK(!x.value());
	CHECK( y.value());
	CHECK(!z.value());
	CHECK( w.value());
}

TEST_CASE("sat-objective-next_solution")
{
	IP ip;
	ip.set_external_solver(IP::Minisat);
	auto x = ip.add_boolean();
	auto y = ip.add_boolean();
	auto z = ip.add_boolean();
	auto w = ip.add_boolean();
	auto u = ip.add_boolean();
	auto v = ip.add_boolean();
	auto obj = 4*x + y + 2*z + w + u + v;
	ip.add_objective(obj);
	ip.add_constraint(x + y + z + w + u + v == 2);
	REQUIRE(ip.solve());

	int num_solutions = 0;
	do {
		REQUIRE(obj.value() == 2);
		num_solutions++;
	} while (ip.next_solution());
	REQUIRE(num_solutions == 6);
}

#endif
