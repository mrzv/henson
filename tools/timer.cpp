#include <string>

#include <mpi.h>

#include <henson/data.h>
#include <henson/data.hpp>
#include <henson/context.h>
namespace h = henson;

#include <henson/time.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <opts/opts.h>

std::string     timer(std::string name)                     { return "timer_" + name; }
std::string     timer_start(std::string name)               { return "timer_start_" + name; }

void            save_time(std::string name, h::time_type t) { h::Value tv = static_cast<size_t>(t); h::save(name, tv); }
h::time_type    load_time(std::string name)                 { return h::get<size_t>(h::load(name)); }

h::time_type    get_henson_timer(std::string name)
{
    if (!henson::exists(timer(name))) return 0;
    return load_time(timer(name));
}

int main(int argc, char** argv)
{
    if (!henson_active())
    {
        fmt::print("{} should run under henson\n", argv[0]);
        return 1;
    }
    MPI_Comm world = henson_get_world();
    int rank;
    MPI_Comm_rank(world, &rank);

    using namespace opts;
    Options ops;

    bool increment, root;
    ops
        >> Option('i', "increment", increment,  "increment the timer value")
        >> Option('r', "root",      root,       "perform comparison on the root node only (and broadcast decision to the rest)");
    ops.parse(argc, argv);

    std::string cmd;
    ops >> PosOption(cmd);
    if (cmd == "start")
    {
        std::string name;
        ops >> PosOption(name);

        save_time(timer_start(name), henson::get_time());
    } else if (cmd == "stop")
    {
        std::string name;
        ops >> PosOption(name);

        h::time_type start = load_time(timer_start(name));
        h::time_type elapsed = henson::get_time() - start;

        if (!increment)
            save_time(timer(name), elapsed);
        else
        {
            h::time_type cur = get_henson_timer(name);
            save_time(timer(name), cur + elapsed);
        }
    } else if (cmd == "reset")
    {
        std::string name;
        ops >> PosOption(name);
        save_time(timer(name), 0);
    } else if (cmd == "<" || cmd == "<=" || cmd == ">" || cmd == ">=")
    {
        std::string name1, name2;
        ops >> PosOption(name1) >> PosOption(name2);

        h::time_type t1 = get_henson_timer(name1),
                     t2 = get_henson_timer(name2);

        int res;
        if (!root || rank == 0)
        {
            if (cmd == "<" && t1 < t2)
                res = 0;
            else if (cmd == "<=" && t1 <= t2)
                res = 0;
            else if (cmd == ">"  && t1 >  t2)
                res = 0;
            else if (cmd == ">=" && t1 >= t2)
                res = 0;
            else
                res = 1;

            if (root)
                MPI_Bcast(&res, 1, MPI_INT, 0, world);
        } else
            MPI_Bcast(&res, 1, MPI_INT, 0, world);

        return res;
    } else if (cmd == "report")
    {
        std::string name;
        ops >> PosOption(name);

        h::time_type start = load_time(timer_start(name));
        h::time_type elapsed = henson::get_time() - start;

        fmt::print("Elapsed: {}\n", h::clock_to_string(elapsed));
    }
}
