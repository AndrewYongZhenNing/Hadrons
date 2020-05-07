/*
 * Application.cpp, part of Hadrons (https://github.com/aportelli/Hadrons)
 *
 * Copyright (C) 2015 - 2020
 *
 * Author: Antonin Portelli <antonin.portelli@me.com>
 *
 * Hadrons is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hadrons is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hadrons.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See the full license in the file "LICENSE" in the top level distribution 
 * directory.
 */

/*  END LEGAL */

#include <Hadrons/Application.hpp>
#include <Hadrons/GeneticScheduler.hpp>
#include <Hadrons/Modules.hpp>

using namespace Grid;
using namespace Hadrons;

#define BIG_SEP "================"
#define SEP     "----------------"

/******************************************************************************
 *                       Application implementation                           *
 ******************************************************************************/
// constructors ////////////////////////////////////////////////////////////////
#define MACOUT(macro)    macro              << " (" << #macro << ")"
#define MACOUTS(macro) HADRONS_STR(macro) << " (" << #macro << ")"

Application::Application(void)
{
    initLogger();
    auto dim = GridDefaultLatt(), mpi = GridDefaultMpi(), loc(dim);

    if (dim.size())
    {
        locVol_ = 1;
        for (unsigned int d = 0; d < dim.size(); ++d)
        {
            loc[d]  /= mpi[d];
            locVol_ *= loc[d];
        }
        LOG(Message) << "====== HADRONS APPLICATION INITIALISATION ======" << std::endl;
        LOG(Message) << "** Dimensions" << std::endl;
        LOG(Message) << "Global lattice: " << dim << std::endl;
        LOG(Message) << "MPI partition : " << mpi << std::endl;
        LOG(Message) << "Local lattice : " << loc << std::endl;
        LOG(Message) << std::endl;
        LOG(Message) << "** Default parameters (and associated C macros)" << std::endl;
        LOG(Message) << "ASCII output precision  : " << MACOUT(DEFAULT_ASCII_PREC) << std::endl;
        LOG(Message) << "Fermion implementation  : " << MACOUTS(FIMPLBASE) << std::endl;
        LOG(Message) << "z-Fermion implementation: " << MACOUTS(ZFIMPLBASE) << std::endl;
        LOG(Message) << "Scalar implementation   : " << MACOUTS(SIMPLBASE) << std::endl;
        LOG(Message) << "Gauge implementation    : " << MACOUTS(GIMPLBASE) << std::endl;
        LOG(Message) << "Eigenvector base size   : " 
                     << MACOUT(HADRONS_DEFAULT_LANCZOS_NBASIS) << std::endl;
        LOG(Message) << "Schur decomposition     : " << MACOUTS(HADRONS_DEFAULT_SCHUR) << std::endl;
        LOG(Message) << std::endl;
    }
}

Application::Application(const Application::GlobalPar &par)
: Application()
{
    setPar(par);
}

Application::Application(const std::string parameterFileName)
: Application()
{
    parameterFileName_ = parameterFileName;
}

// access //////////////////////////////////////////////////////////////////////
void Application::setPar(const Application::GlobalPar &par)
{
    par_ = par;
    setupDatabase();
}

const Application::GlobalPar & Application::getPar(void)
{
    return par_;
}

// module creation /////////////////////////////////////////////////////////////
void Application::createModule(const std::string name, const std::string type, 
                               XmlReader &reader)
{
    vm().createModule(name, type, reader);
    if (db_.isConnected())
    {
        ModuleEntry m;

        m.moduleId     = vm().getModuleAddress(name);
        m.name         = name;
        m.moduleTypeId = dbInsertModuleType(vm().getModuleType(name));
        m.parameters   = vm().getModule(name)->parString();
        db_.insert("modules", m);
    }
}

// execute /////////////////////////////////////////////////////////////////////
void Application::run(void)
{
    LOG(Message) << "====== HADRONS APPLICATION START ======" << std::endl;
    if (!parameterFileName_.empty() and (vm().getNModule() == 0))
    {
        parseParameterFile(parameterFileName_);
    }
    if (getPar().runId.empty())
    {
        HADRONS_ERROR(Definition, "run id is empty");
    }
    LOG(Message) << "RUN ID '" << getPar().runId << "'" << std::endl;
    BinaryIO::latticeWriteMaxRetry = getPar().parallelWriteMaxRetry;
    LOG(Message) << "Attempt(s) for resilient parallel I/O: " 
                 << BinaryIO::latticeWriteMaxRetry << std::endl;
    vm().setRunId(getPar().runId);
    vm().printContent();
    env().printContent();
    if (getPar().saveSchedule or getPar().scheduleFile.empty())
    {
        schedule();
        if (getPar().saveSchedule)
        {
            std::string filename;

            filename = (getPar().scheduleFile.empty()) ? 
                         "hadrons.sched" : getPar().scheduleFile;
            saveSchedule(filename);
        }
    }
    else
    {
        loadSchedule(getPar().scheduleFile);
    }
    printSchedule();
    if (!getPar().graphFile.empty())
    {
        makeFileDir(getPar().graphFile, env().getGrid());
        vm().dumpModuleGraph(getPar().graphFile);
    }
    configLoop();
}

// parse parameter file ////////////////////////////////////////////////////////
void Application::parseParameterFile(const std::string parameterFileName)
{
    XmlReader reader(parameterFileName);
    GlobalPar par;
    ObjectId  id;
    
    LOG(Message) << "Building application from '" << parameterFileName << "'..." << std::endl;
    read(reader, "parameters", par);
    setPar(par);
    if (!push(reader, "modules"))
    {
        HADRONS_ERROR(Parsing, "Cannot open node 'modules' in parameter file '" 
                              + parameterFileName + "'");
    }
    if (!push(reader, "module"))
    {
        HADRONS_ERROR(Parsing, "Cannot open node 'modules/module' in parameter file '" 
                              + parameterFileName + "'");
    }
    do
    {
        read(reader, "id", id);
        createModule(id.name, id.type, reader);
    } while (reader.nextElement("module"));
    pop(reader);
    pop(reader);
}

void Application::saveParameterFile(const std::string parameterFileName, unsigned int prec)
{
    LOG(Message) << "Saving application to '" << parameterFileName << "'..." << std::endl;
    if (env().getGrid()->IsBoss())
    {
        XmlWriter          writer(parameterFileName);
        ObjectId           id;
        const unsigned int nMod = vm().getNModule();

        writer.setPrecision(prec);
        write(writer, "parameters", getPar());
        push(writer, "modules");
        for (unsigned int i = 0; i < nMod; ++i)
        {
            push(writer, "module");
            id.name = vm().getModuleName(i);
            id.type = vm().getModule(i)->getRegisteredName();
            write(writer, "id", id);
            vm().getModule(i)->saveParameters(writer, "options");
            pop(writer);
        }
        pop(writer);
        pop(writer);
    }
}

// schedule computation ////////////////////////////////////////////////////////
void Application::schedule(void)
{
    if (!scheduled_ and !loadedSchedule_)
    {
        program_   = vm().schedule(par_.genetic);
        if (db_.isConnected())
        {
            const VirtualMachine::MemoryProfile &p = vm().getMemoryProfile();

            for (unsigned int i = 0; i < p.object.size(); ++i)
            {
                ObjectEntry o;

                o.objectId     = i;
                o.name         = env().getObjectName(i);
                o.objectTypeId = dbInsertObjectType(env().getObjectDerivedType(i),
                                                    env().getObjectType(i));
                o.size         = p.object[i].size;
                o.moduleId     = p.object[i].module;
                db_.insert("objects", o);
            }
            for (unsigned int i = 0; i < program_.size(); ++i)
            {
                ScheduleEntry s;

                s.step     = i;
                s.moduleId = program_[i];
                db_.insert("schedule", s);
            }
        }
        scheduled_ = true;
    }
}

void Application::saveSchedule(const std::string filename)
{
    LOG(Message) << "Saving current schedule to '" << filename << "'..."
                 << std::endl;
    if (env().getGrid()->IsBoss())
    {
        TextWriter               writer(filename);
        std::vector<std::string> program;
        
        if (!scheduled_)
        {
            HADRONS_ERROR(Definition, "Computation not scheduled");
        }

        for (auto address: program_)
        {
            program.push_back(vm().getModuleName(address));
        }
        write(writer, "schedule", program);
    }
}

void Application::loadSchedule(const std::string filename)
{
    TextReader               reader(filename);
    std::vector<std::string> program;
    
    LOG(Message) << "Loading schedule from '" << filename << "'..."
                 << std::endl;
    read(reader, "schedule", program);
    program_.clear();
    for (auto &name: program)
    {
        program_.push_back(vm().getModuleAddress(name));
    }
    loadedSchedule_ = true;
    scheduled_      = true;
}

void Application::printSchedule(void)
{
    if (!scheduled_ and !loadedSchedule_)
    {
        HADRONS_ERROR(Definition, "Computation not scheduled");
    }
    auto peak = vm().memoryNeeded(program_);
    LOG(Message) << "Schedule (memory needed: " << sizeString(peak) << "):"
                 << std::endl;
    for (unsigned int i = 0; i < program_.size(); ++i)
    {
        LOG(Message) << std::setw(4) << i + 1 << ": "
                     << vm().getModuleName(program_[i]) << std::endl;
    }
}

// loop on configurations //////////////////////////////////////////////////////
void Application::configLoop(void)
{
    auto range = par_.trajCounter;
    
    for (unsigned int t = range.start; t < range.end; t += range.step)
    {
        LOG(Message) << BIG_SEP << " Starting measurement for trajectory " << t
                     << " " << BIG_SEP << std::endl;
        vm().setTrajectory(t);
        vm().executeProgram(program_);
    }
    LOG(Message) << BIG_SEP << " End of measurement " << BIG_SEP << std::endl;
    env().freeAll();
}

// database initialisation /////////////////////////////////////////////////////
void Application::setupDatabase(void)
{
    if (!getPar().databaseFile.empty())
    {
        LOG(Message) << "Setting up SQLite database in file '" 
                     << getPar().databaseFile << "'..." << std::endl;
        db_.setFilename(getPar().databaseFile, env().getGrid());
        db_.execute("PRAGMA foreign_keys = ON;");
        if (!db_.tableExists("global"))
        {
            db_.createTable<GlobalEntry>("global");
        }
        if (!db_.tableExists("moduleTypes"))
        {
            db_.createTable<ModuleTypeEntry>("moduleTypes", "PRIMARY KEY(moduleTypeId)");
        }
        if (!db_.tableExists("modules"))
        {
            db_.createTable<ModuleEntry>("modules", "PRIMARY KEY(moduleId)"
                "FOREIGN KEY(moduleTypeId) REFERENCES moduleTypes(moduleTypeId)");
        }
        if (!db_.tableExists("objectTypes"))
        {
            db_.createTable<ObjectTypeEntry>("objectTypes", "PRIMARY KEY(objectTypeId)");
        }
        if (!db_.tableExists("objects"))
        {
            db_.createTable<ObjectEntry>("objects", "PRIMARY KEY(objectId)," 
                "FOREIGN KEY(moduleId) REFERENCES modules(moduleId),"
                "FOREIGN KEY(objectTypeId) REFERENCES objectTypes(objectTypeId)");
        }
        if (!db_.tableExists("schedule"))
        {
            db_.createTable<ScheduleEntry>("schedule", "PRIMARY KEY(step)," 
                "FOREIGN KEY(moduleId) REFERENCES modules(moduleId)");
        }
        db_.execute(
            "CREATE VIEW IF NOT EXISTS vModules AS                                     "
            "SELECT moduleId,                                                          "
            "       modules.name,                                                      "
            "       moduleTypes.type AS type,                                          "
            "       modules.parameters                                                 "
            "FROM modules                                                              "
            "INNER JOIN moduleTypes ON modules.moduleTypeId = moduleTypes.moduleTypeId "
            "ORDER BY moduleId;                                                        "
        );
        db_.execute(
            "CREATE VIEW IF NOT EXISTS vObjects AS                                     "
            "SELECT objectId,                                                          "
            "       objects.name,                                                      "
            "       objectTypes.type AS type,                                          "
            "       objectTypes.baseType AS baseType,                                  "
            "       objects.size*1.0/1024/1024 AS sizeMB,                              "
            "       modules.name AS module                                             "
            "FROM objects                                                              "
            "INNER JOIN objectTypes ON objects.objectTypeId = objectTypes.objectTypeId "
            "INNER JOIN modules     ON objects.moduleId = modules.moduleId             "
            "ORDER BY objectId;                                                        "
        );
        db_.execute(
            "CREATE VIEW IF NOT EXISTS vSchedule AS                                    "
            "SELECT step,                                                              "
            "       modules.name AS module                                             "
            "FROM schedule                                                             "
            "INNER JOIN modules ON schedule.moduleId = modules.moduleId                "
            "ORDER BY step;                                                            "
        );
    }
}

unsigned int Application::dbInsertModuleType(const std::string type)
{
    QueryResult r = db_.execute("SELECT moduleTypeId FROM moduleTypes "
                                "WHERE type = '" + type + "';");

    if (r.rows() == 0)
    {
        ModuleTypeEntry e;

        r = db_.execute("SELECT COUNT(*) FROM moduleTypes;");
        e.moduleTypeId = std::stoi(r[0][0]);
        e.type         = type;
        db_.insert("moduleTypes", e);

        return e.moduleTypeId;
    }
    else
    {
        return std::stoi(r[0][0]);
    }
}

unsigned int Application::dbInsertObjectType(const std::string type, 
                                             const std::string baseType)
{
    QueryResult r = db_.execute("SELECT objectTypeId FROM objectTypes "
                                "WHERE type = '" + type + "' "
                                "AND baseType = '" + baseType + "';");

    if (r.rows() == 0)
    {
        ObjectTypeEntry e;

        r = db_.execute("SELECT COUNT(*) FROM objectTypes;");
        e.objectTypeId = std::stoi(r[0][0]);
        e.type         = type;
        e.baseType     = baseType;
        db_.insert("objectTypes", e);

        return e.objectTypeId;
    }
    else
    {
        return std::stoi(r[0][0]);
    }
}
