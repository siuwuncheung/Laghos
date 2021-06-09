#include <fstream>
#include <iostream>
#include <sstream>

#include "laghos_utils.hpp"

using namespace std;

void split_line(const std::string &line, std::vector<std::string> &words)
{
    words.clear();
    std::istringstream iss(line);
    std::string new_word;
    while (std::getline(iss, new_word, ' ')) {
        words.push_back(new_word);
    }
}

void DeleteROMSolution(std::string outputPath)
{
    bool last_step = false;
    for (int ti = 1; !last_step; ti++)
    {
        std::string filename = outputPath + "/ROMsol/romS_" + std::to_string(ti);
        std::ifstream infile_romS(filename.c_str());
        if (infile_romS.good())
        {
            infile_romS.close();
            remove(filename.c_str());
        }
        else
        {
            last_step = true;
            break;
        }
    }
}

void ReadGreedyPhase(bool& rom_offline, bool& rom_online, bool& rom_restore, bool& rom_calc_rel_error,
                     ROM_Options& romOptions, std::string greedyfile)
{
    std::ifstream greedystream(greedyfile);
    if (!greedystream.is_open())
    {
        rom_online = true;
        romOptions.hyperreduce_prep = true;
        romOptions.hyperreduce = false;
        rom_calc_rel_error = false;
    }
    else
    {
        std::string line;
        std::vector<std::string> words;
        std::getline(greedystream, line);
        split_line(line, words);

        if (words[0] == "online")
        {
            if (words[1] == "hyperreduce_prep")
            {
                rom_online = true;
                rom_calc_rel_error = false;
            }
            else if (words[1] == "hyperreduce")
            {
                rom_restore = true;
                romOptions.hyperreduce = false;
            }
        }
    }
}

void WriteGreedyPhase(bool& rom_offline, bool& rom_online, bool& rom_restore,
                      ROM_Options& romOptions, std::string greedyfile)
{
    std::ofstream greedyout(greedyfile);
    if (rom_online)
    {
        greedyout << "online" << " ";
    }
    else if (rom_restore)
    {
        greedyout << "restore" << " ";
    }
    else
    {
        greedyout << "offline" << " ";
    }
    if (romOptions.hyperreduce_prep)
    {
        greedyout << "hyperreduce_prep" << endl;
    }
    else if (romOptions.hyperreduce)
    {
        greedyout << "hyperreduce" << endl;
    }
    else
    {
        greedyout << "nonreduced" << endl;
    }
    greedyout.close();
}

int WriteOfflineParam(int dim, double dt, ROM_Options& romOptions,
                      const int numWindows, const char* twfile, std::string paramfile, const bool printStatus)
{
    if (romOptions.parameterID <= 0)
    {
        if (printStatus)
        {
            std::ofstream opout(paramfile);
            opout << dim << " ";
            opout << dt << " ";
            opout << romOptions.useOffset << " ";
            opout << romOptions.offsetType << " ";
            opout << romOptions.SNS << " ";
            opout << numWindows << " ";
            opout << romOptions.VTos << " ";
            opout << twfile << endl;
            opout.close();
        }
    }
    else
    {
        VerifyOfflineParam(dim, dt, romOptions, numWindows, twfile, paramfile, true);
    }

    std::ofstream opout(paramfile, std::fstream::app);
    if (printStatus)
    {
        opout << romOptions.parameterID << " ";
        opout << romOptions.rhoFactor << " ";
        opout << romOptions.blast_energyFactor << " ";
        opout << romOptions.atwoodFactor << endl;
        opout.close();
    }
}

int VerifyOfflineParam(int& dim, double& dt, ROM_Options& romOptions,
                       const int numWindows, const char* twfile, std::string paramfile, const bool rom_offline)
{
    std::ifstream opin(paramfile);
    MFEM_VERIFY(opin.is_open(), "Offline parameter record file does not exist.");

    std::string line;
    std::vector<std::string> words;
    std::getline(opin, line);
    split_line(line, words);

    MFEM_VERIFY(std::stoi(words[2]) == romOptions.useOffset, "-romos option does not match record.");
    MFEM_VERIFY(std::stoi(words[3]) == romOptions.offsetType, "-romostype option does not match record.");
    MFEM_VERIFY(std::stoi(words[4]) == romOptions.SNS, "-romsns option does not match record.");

    if (rom_offline)
    {
        MFEM_VERIFY(std::stoi(words[5]) == numWindows, "-nwin option does not match record.");
        MFEM_VERIFY(std::strcmp(words[7].c_str(), twfile) == 0, "-tw option does not match record.");
    }
    else
    {
        dim = std::stoi(words[0]);
        dt = std::stod(words[1]);
        romOptions.VTos = std::stod(words[6]);
    }

    opin.close();

    return 0;
}

void BasisGeneratorFinalSummary(CAROM::BasisGenerator* bg, const double energyFraction, int & cutoff, const std::string cutoffOutputPath, const bool printout)
{
    const int rom_dim = bg->getSpatialBasis()->numColumns();
    const CAROM::Vector* sing_vals = bg->getSingularValues();

    MFEM_VERIFY(rom_dim <= sing_vals->dim(), "");

    double sum = 0.0;
    for (int sv = 0; sv < sing_vals->dim(); ++sv) {
        sum += (*sing_vals)(sv);
    }

    vector<double> energy_fractions = {0.9999, 0.999, 0.99, 0.9};
    bool reached_cutoff = false;

    double partialSum = 0.0;
    for (int sv = 0; sv < sing_vals->dim(); ++sv) {
        partialSum += (*sing_vals)(sv);
        if (printout)
        {
            for (int i = energy_fractions.size() - 1; i >= 0; i--)
            {
                if (partialSum / sum > energy_fractions[i])
                {
                    cout << "For energy fraction: " << energy_fractions[i] << ", take first "
                         << sv+1 << " of " << sing_vals->dim() << " basis vectors" << endl;
                    if (cutoffOutputPath != "")
                    {
                        writeNum(sv+1, cutoffOutputPath + "_" + to_string(energy_fractions[i]));
                    }
                    energy_fractions.pop_back();
                }
                else
                {
                    break;
                }
            }
        }
        if (!reached_cutoff && partialSum / sum > energyFraction)
        {
            cutoff = sv+1;
            reached_cutoff = true;
        }
    }

    if (printout) cout << "Take first " << cutoff << " of " << sing_vals->dim() << " basis vectors" << endl;
}

void PrintSingularValues(const int rank, const std::string& basename, const std::string& name, CAROM::BasisGenerator* bg, const bool usingWindows, const int window)
{
    const CAROM::Vector* sing_vals = bg->getSingularValues();

    char tmp[100];
    sprintf(tmp, ".%06d", rank);

    std::string fullname = (usingWindows) ? basename + "/sVal" + name + std::to_string(window) + tmp : basename + "/sVal" + name + tmp;

    std::ofstream ofs(fullname.c_str(), std::ofstream::out);
    ofs.precision(16);

    for (int sv = 0; sv < sing_vals->dim(); ++sv) {
        ofs << (*sing_vals)(sv) << endl;
    }

    ofs.close();
}

int ReadTimesteps(std::string const& path, std::vector<double>& time)
{
    int nsteps = 0;
    std::string nsfile(path + "/num_steps");

    {
        std::ifstream ifsn(nsfile.c_str());
        ifsn >> nsteps;
        ifsn.close();
    }

    MFEM_VERIFY(nsteps > 0, "");
    time.assign(nsteps, -1.0);

    std::string tsfile(path + "/timesteps.csv");
    std::ifstream ifs(tsfile.c_str());

    if (!ifs.is_open())
    {
        cout << "Error: invalid file" << endl;
        return 1;  // invalid file
    }

    string line, word;
    int count = 0;
    while (getline(ifs, line))
    {
        if (count >= nsteps)
        {
            cout << "Error reading CSV file. Read more than " << nsteps << " lines" << endl;
            ifs.close();
            return 3;
        }

        stringstream s(line);
        vector<string> row;

        while (getline(s, word, ','))
            row.push_back(word);

        if (row.size() != 1)
        {
            cout << "Error: CSV file does not specify exactly 1 parameter" << endl;
            ifs.close();
            return 2;  // incorrect number of parameters
        }

        time[count++] = stod(row[0]);
    }

    ifs.close();

    if (count != nsteps)
    {
        cout << "Error reading CSV file. Read " << count << " lines but expected " << nsteps << endl;
        return 3;
    }

    return 0;
}

int ReadTimeWindows(const int nw, std::string twfile, Array<double>& twep, const bool printStatus)
{
    if (printStatus) cout << "Reading time windows from file " << twfile << endl;

    std::ifstream ifs(twfile.c_str());

    if (!ifs.is_open())
    {
        cout << "Error: invalid file" << endl;
        return 1;  // invalid file
    }

    twep.SetSize(nw);

    string line, word;
    int count = 0;
    while (getline(ifs, line))
    {
        if (count >= nw)
        {
            cout << "Error reading CSV file. Read more than " << nw << " lines" << endl;
            ifs.close();
            return 3;
        }

        stringstream s(line);
        vector<string> row;

        while (getline(s, word, ','))
            row.push_back(word);

        if (row.size() != 1)
        {
            cout << "Error: CSV file does not specify exactly 1 parameter" << endl;
            ifs.close();
            return 2;  // incorrect number of parameters
        }

        twep[count] = stod(row[0]);

        if (printStatus) cout << "Using time window " << count << " with end time " << twep[count] << endl;
        count++;
    }

    ifs.close();

    if (count != nw)
    {
        cout << "Error reading CSV file. Read " << count << " lines but expected " << nw << endl;
        return 3;
    }

    return 0;
}

int ReadTimeWindowParameters(const int nw, std::string twfile, Array<double>& twep, Array2D<int>& twparam, double sFactor[], const bool printStatus, const bool sns)
{
    if (printStatus) cout << "Reading time window parameters from file " << twfile << endl;

    std::ifstream ifs(twfile.c_str());

    if (!ifs.is_open())
    {
        cout << "Error: invalid file" << endl;
        return 1;  // invalid file
    }

    // Parameters to read for each time window:
    // end time, rdimx, rdimv, rdime
    const int nparamRead = sns ? 4 : 6; // number of parameters to read for each time window

    // Add 3 more parameters for nsamx, nsamv, nsame
    const int nparam = nparamRead + 3;

    twep.SetSize(nw);
    twparam.SetSize(nw, nparam - 1);

    string line, word;
    int count = 0;
    while (getline(ifs, line))
    {
        if (count >= nw)
        {
            cout << "Error reading CSV file. Read more than " << nw << " lines" << endl;
            ifs.close();
            return 3;
        }

        stringstream s(line);
        vector<string> row;

        while (getline(s, word, ','))
            row.push_back(word);

        if (row.size() != nparamRead)
        {
            cout << "Error: CSV file does not specify " << nparamRead << " parameters" << endl;
            ifs.close();
            return 2;  // incorrect number of parameters
        }

        twep[count] = stod(row[0]);
        for (int i=0; i<nparamRead-1; ++i)
            twparam(count,i) = stoi(row[i+1]);

        // Setting nsamx, nsamv, nsame
        twparam(count, nparamRead-1) = sFactor[0] * twparam(count, 0);
        twparam(count, nparamRead)   = sFactor[1] * twparam(count, sns ? 1 : 3);
        twparam(count, nparamRead+1) = sFactor[2] * twparam(count, sns ? 2 : 4);

        if (printStatus)
        {
            if (sns) cout << "Using time window " << count << " with end time " << twep[count] << ", rdimx " << twparam(count,0)
                              << ", rdimv " << twparam(count,1) << ", rdime " << twparam(count,2) << ", nsamx " << twparam(count,3)
                              << ", nsamv " << twparam(count,4) << ", nsame " << twparam(count,5) << endl;
            else cout << "Using time window " << count << " with end time " << twep[count] << ", rdimx " << twparam(count,0)
                          << ", rdimv " << twparam(count,1) << ", rdime " << twparam(count,2) << ", rdimfv " << twparam(count,3)
                          << ", rdimfe " << twparam(count,4) << ", nsamx " << twparam(count,5)
                          << ", nsamv " << twparam(count,6) << ", nsame " << twparam(count,7) << endl;
        }

        count++;
    }

    ifs.close();

    if (count != nw)
    {
        cout << "Error reading CSV file. Read " << count << " lines but expected " << nw << endl;
        return 3;
    }

    return 0;
}

void SetWindowParameters(Array2D<int> const& twparam, ROM_Options & romOptions)
{
    const int w = romOptions.window;
    romOptions.dimX = min(romOptions.max_dimX, twparam(w,0));
    romOptions.dimV = min(romOptions.max_dimV, twparam(w,1));
    romOptions.dimE = min(romOptions.max_dimE, twparam(w,2));
    romOptions.dimFv = romOptions.SNS ? romOptions.dimV : min(romOptions.max_dimFv, twparam(w,3));
    romOptions.dimFe = romOptions.SNS ? romOptions.dimE : min(romOptions.max_dimFe, twparam(w,4));

    const int oss = (romOptions.SNS) ? 3 : 5;
    romOptions.sampX = twparam(w,oss);
    romOptions.sampV = twparam(w,oss+1);
    romOptions.sampE = twparam(w,oss+2);
}

void AppendPrintParGridFunction(std::ofstream *ofs, ParGridFunction *gf)
{
    Vector tv(gf->ParFESpace()->GetTrueVSize());
    gf->GetTrueDofs(tv);

    for (int i=0; i<tv.Size(); ++i)
        *ofs << tv[i] << std::endl;
}

void PrintParGridFunction(const int rank, const std::string& name, ParGridFunction *gf)
{
    Vector tv(gf->ParFESpace()->GetTrueVSize());
    gf->GetTrueDofs(tv);

    char tmp[100];
    sprintf(tmp, ".%06d", rank);

    std::string fullname = name + tmp;

    std::ofstream ofs(fullname.c_str(), std::ofstream::out);
    ofs.precision(16);

    for (int i=0; i<tv.Size(); ++i)
        ofs << tv[i] << std::endl;

    ofs.close();
}

double PrintDiffParGridFunction(NormType normtype, const int rank, const std::string& name, ParGridFunction *gf)
{
    Vector tv(gf->ParFESpace()->GetTrueVSize());

    char tmp[100];
    sprintf(tmp, ".%06d", rank);

    std::string fullname = name + tmp;

    std::ifstream ifs(fullname.c_str());

    for (int i=0; i<tv.Size(); ++i)
    {
        double d;
        ifs >> d;
        tv[i] = d;
    }

    ifs.close();

    ParGridFunction rgf(gf->ParFESpace());
    rgf.SetFromTrueDofs(tv);

    return PrintNormsOfParGridFunctions(normtype, rank, name, &rgf, gf, true);
}

void writeNum(int num, std::string file_name) {
    ofstream file;
    file.open(file_name);
    file << num << endl;
    file.close();
}

// read data from from text.txt and store it in vector v
void readNum(int& num, std::string file_name) {
    ifstream file;
    file.open(file_name);
    string line;
    getline(file, line);
    num = stoi(line);
    file.close();
}

void writeDouble(double num, std::string file_name) {
    ofstream file;
    file.open(file_name);
    file << std::fixed << std::setprecision(16) << num <<endl;
    file.close();
}

// read data from from text.txt and store it in vector v
void readDouble(double& num, std::string file_name) {
    ifstream file;
    file.open(file_name);
    string line;
    getline(file, line);
    num = stod(line);
    file.close();
}

void writeVec(vector<int> v, std::string file_name) {
    ofstream file;
    file.open(file_name);
    for(int i=0; i<v.size(); ++i) {
        file << v[i] << endl;
    }
    file.close();
}

// read data from from text.txt and store it in vector v
void readVec(vector<int> &v, std::string file_name) {
    ifstream file;
    file.open(file_name);
    string line;
    while(getline(file, line)) {
        v.push_back(stoi(line));
    }
    file.close();
}

double PrintNormsOfParGridFunctions(NormType normtype, const int rank, const std::string& name, ParGridFunction *f1, ParGridFunction *f2,
                                    const bool scalar)
{
    ConstantCoefficient zero(0.0);
    Vector zerov(3);
    zerov = 0.0;
    VectorConstantCoefficient vzero(zerov);

    double fomloc, romloc, diffloc;

    // TODO: why does ComputeL2Error call the local GridFunction version rather than the global ParGridFunction version?
    // Only f2->ComputeL2Error calls the ParGridFunction version.
    if (scalar)
    {
        switch(normtype)
        {
        case l1norm:
            fomloc = f1->ComputeL1Error(zero);
            romloc = f2->ComputeL1Error(zero);
            break;
        case l2norm:
            fomloc = f1->ComputeL2Error(zero);
            romloc = f2->ComputeL2Error(zero);
            break;
        case maxnorm:
            fomloc = f1->ComputeMaxError(zero);
            romloc = f2->ComputeMaxError(zero);
            break;
        }
    }
    else
    {
        switch(normtype)
        {
        case l1norm:
            fomloc = f1->ComputeL1Error(vzero);
            romloc = f2->ComputeL1Error(vzero);
            break;
        case l2norm:
            fomloc = f1->ComputeL2Error(vzero);
            romloc = f2->ComputeL2Error(vzero);
            break;
        case maxnorm:
            fomloc = f1->ComputeMaxError(vzero);
            romloc = f2->ComputeMaxError(vzero);
            break;
        }
    }

    *f1 -= *f2;  // works because GridFunction is derived from Vector

    if (scalar)
    {
        switch(normtype)
        {
        case l1norm:
            diffloc = f1->ComputeL1Error(zero);
            break;
        case l2norm:
            diffloc = f1->ComputeL2Error(zero);
            break;
        case maxnorm:
            diffloc = f1->ComputeMaxError(zero);
            break;
        }
    }
    else
    {
        switch(normtype)
        {
        case l1norm:
            diffloc = f1->ComputeL1Error(vzero);
            break;
        case l2norm:
            diffloc = f1->ComputeL2Error(vzero);
            break;
        case maxnorm:
            diffloc = f1->ComputeMaxError(vzero);
            break;
        }
    }

    double fomloc2 = fomloc*fomloc;
    double romloc2 = romloc*romloc;
    double diffloc2 = diffloc*diffloc;

    double fomglob2, romglob2, diffglob2;

    // TODO: is this right? The "loc" norms should be global, but they are not.
    MPI_Allreduce(&fomloc2, &fomglob2, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&romloc2, &romglob2, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&diffloc2, &diffglob2, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    /*
    fomglob2 = fomloc2;
    romglob2 = romloc2;
    diffglob2 = diffloc2;
    */

    double FOMnorm = sqrt(fomglob2);
    double ROMnorm = sqrt(romglob2);
    double DIFFnorm = sqrt(diffglob2);
    double relDIFFnorm = sqrt(diffglob2)/sqrt(fomglob2);

    if (rank == 0)
    {
        switch(normtype)
        {
        case l1norm:
            cout << "L1 norm error:" << endl;
            break;
        case l2norm:
            cout << "L2 norm error:" << endl;
            break;
        case maxnorm:
            cout << "MAX norm error:" << endl;
            break;
        }

        cout << rank << ": " << name << " FOM norm " << FOMnorm << endl;
        cout << rank << ": " << name << " ROM norm " << ROMnorm << endl;
        cout << rank << ": " << name << " DIFF norm " << DIFFnorm << endl;
        cout << rank << ": " << name << " Rel. DIFF norm " << relDIFFnorm << endl;
    }

    char tmp[100];
    sprintf(tmp, ".%06d", rank);

    std::string fullname = name + "_norms" + tmp;

    std::ofstream ofs(fullname.c_str(), std::ofstream::out);
    ofs.precision(16);

    ofs << "FOM norm " << FOMnorm << endl;
    ofs << "ROM norm " << ROMnorm << endl;
    ofs << "DIFF norm " << DIFFnorm << endl;
    ofs << "Rel. DIFF norm " << relDIFFnorm << endl;

    ofs.close();

    return relDIFFnorm;

}

void PrintL2NormsOfParGridFunctions(const int rank, const std::string& name, ParGridFunction *f1, ParGridFunction *f2,
                                    const bool scalar)
{
    ConstantCoefficient zero(0.0);
    Vector zerov(3);
    zerov = 0.0;
    VectorConstantCoefficient vzero(zerov);

    double fomloc, romloc, diffloc;

    // TODO: why does ComputeL2Error call the local GridFunction version rather than the global ParGridFunction version?
    // Only f2->ComputeL2Error calls the ParGridFunction version.
    if (scalar)
    {
        fomloc = f1->ComputeL2Error(zero);
        romloc = f2->ComputeL2Error(zero);
    }
    else
    {
        fomloc = f1->ComputeL2Error(vzero);
        romloc = f2->ComputeL2Error(vzero);
    }

    *f1 -= *f2;  // works because GridFunction is derived from Vector

    if (scalar)
    {
        diffloc = f1->ComputeL2Error(zero);
    }
    else
    {
        diffloc = f1->ComputeL2Error(vzero);
    }

    double fomloc2 = fomloc*fomloc;
    double romloc2 = romloc*romloc;
    double diffloc2 = diffloc*diffloc;

    double fomglob2, romglob2, diffglob2;

    // TODO: is this right? The "loc" norms should be global, but they are not.
    MPI_Allreduce(&fomloc2, &fomglob2, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&romloc2, &romglob2, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&diffloc2, &diffglob2, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    /*
    fomglob2 = fomloc2;
    romglob2 = romloc2;
    diffglob2 = diffloc2;
    */

    cout << rank << ": " << name << " FOM norm " << sqrt(fomglob2) << endl;
    cout << rank << ": " << name << " ROM norm " << sqrt(romglob2) << endl;
    cout << rank << ": " << name << " DIFF norm " << sqrt(diffglob2) << endl;
    cout << rank << ": " << name << " Rel. DIFF norm " << sqrt(diffglob2)/sqrt(fomglob2) << endl;
}