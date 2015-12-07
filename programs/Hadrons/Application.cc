/*
 * Application.cc, part of Grid
 *
 * Copyright (C) 2015 Antonin Portelli
 *
 * Grid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Grid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Grid.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Hadrons/Application.hpp>
#include <Hadrons/Graph.hpp>

using namespace std;
using namespace Grid;
using namespace Hadrons;

/******************************************************************************
 *                       Application implementation                           *
 ******************************************************************************/
// constructor /////////////////////////////////////////////////////////////////
Application::Application(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "usage: " << argv[0] << " <parameter file> [Grid options]";
        cerr << endl;
        exit(EXIT_FAILURE);
    }
    parameterFileName_ = argv[1];
    Grid_init(&argc, &argv);
    HadronsLogError.Active(GridLogError.isActive());
    HadronsLogWarning.Active(GridLogWarning.isActive());
    HadronsLogMessage.Active(GridLogMessage.isActive());
    HadronsLogDebug.Active(GridLogDebug.isActive());
    LOG(Message) << "Grid initialized" << endl;
}

// destructor //////////////////////////////////////////////////////////////////
Application::~Application(void)
{
    LOG(Message) << "Grid is finalizing now" << endl;
    Grid_finalize();
}

// execute /////////////////////////////////////////////////////////////////////
void Application::run(void)
{
    Graph<string> g;
    
    g.addEdge("A", "B");
    g.addEdge("B", "D");
    g.addEdge("B", "X1");
    g.addEdge("X1", "X2");
    g.addEdge("D", "E");
    g.addEdge("E", "C");
    g.addEdge("Z", "Y");
    g.addEdge("Z", "W");
    g.addEdge("Z", "R");
    g.addEdge("W", "R");
    g.addEdge("U", "I");
    
    cout << g << endl;
    auto vec = g.getConnectedComponents();
    for (auto &h: vec)
    {
        cout << h << endl;
    }
    for (auto &h: vec)
    {
        auto top = h.allTopoSort();
        for (auto &s: top)
        {
            for (auto &v: s)
            {
                cout << v << " ";
            }
            cout << endl;
        }
        cout << "--------" << endl;
    }
}

// parse parameter file ////////////////////////////////////////////////////////
void Application::parseParameterFile(void)
{
    
}
