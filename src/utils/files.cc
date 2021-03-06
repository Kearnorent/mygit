#include <fnmatch.h>
#include <list>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fstream>

#include "utils/zlib.hh"
#include "utils/utils.hh"
#include "utils/wrappers.hh"
#include "utils/get_paths.hh"

#include "objects/blob.hh"

namespace utils
{
    bool IsDirExists(const std::string &path)
    {
        struct stat st;
        if (stat(path.c_str(), &st) != 0)
            return false;
        return (st.st_mode & S_IFDIR) != 0;
    }

    bool IsFileExists(const std::string &path)
    {
        struct stat st;
        if (stat(path.c_str(), &st) != 0)
            return false;
        return (st.st_mode & S_IFREG) != 0;
    }

    bool CreateDir(const std::string &name)
    {
        if (mkdir(name.c_str(), 0777) == -1)
        {
            return false;
        }
        return true;
    }

    bool CreateFile(const std::string &name)
    {
        std::ofstream file { name };
        return true;
    }

    std::string FindPathToRootRepo()
    {
        std::string currentPath = "./";
        while (true)
        {
            DIR *dir = opendir(currentPath.c_str());
            if (dir == nullptr)
                break;
            else
                closedir(dir);
            if (IsDirExists(currentPath + "/" + g_DB_FILE + "/"))
                return currentPath;
            else
                currentPath += "../";
        }
        return "";
    }

    std::string ReadFile (const std::string& path)
    {
        std::ifstream ifs(path);
        std::string contents((std::istreambuf_iterator<char>(ifs)),
                              (std::istreambuf_iterator<char>()));
        return contents;
    }

    void WriteFile (const std::string& path, const std::string& str)
    {
        std::ofstream myfile;
        myfile.open (path);
        myfile << str;
        myfile.close();
    }

    std::string CleanPath (const std::string& path)
    {
        std::string newPath;
        size_t i = 0;
        while(i < path.size())
        {
            if (i + 2 < path.size() and path[i] == '.' and path[i + 1] == '.' and path[i + 2] == '/')
            {
                newPath += "../";
                i += 3;
                utils::GoToCharAfterNextSlash(path, i);
            }
            else if (i + 1 < path.size() and path[i] == '.' and path[i + 1] == '/')
            {
                i += 2;
                utils::GoToCharAfterNextSlash(path, i);
            }
            else if (i + 1 < path.size() and path[i] == '/' and path[i + 1] == '/')
            {
                if (not newPath.empty())
                    newPath += "/";
                utils::GoToCharAfterNextSlash(path, i);
            }
            else
                newPath += path[i++];
        }
        if (newPath.empty() or newPath == ".")
            return "./";
        return newPath;
    }

    std::string GetPathRelativeToDotMyGit(const std::string& pathToFileCpy)
    {
        std::string origin = GetCwd();

        std::string pathToFile = pathToFileCpy;
        std::string filename = CutFileInPath(pathToFile);

        if (pathToFile.empty())
            pathToFile = ".";

        /// Create dirs if not already here
        std::string dirToRemove = CreateDirectoriesAboveFileReturnFirstToDelete(CleanPath(pathToFileCpy));

        utils::ChangeDirWrapper(pathToFile);

        std::string cwd = GetCwd();

        utils::ChangeDirWrapper(utils::PathToRootRepo());

        std::string rootWD = GetCwd();

        utils::ChangeDirWrapper(origin);

        /// Remove potential dirs that were created previously
        if (not dirToRemove.empty())
        {
            std::string command = "rm -rf " + dirToRemove;
            int pid = system(command.c_str());
            int status;
            waitpid(pid, &status, 0);
        }

        if (cwd == rootWD)
            return CleanPath(filename);

        size_t ind_diff = cwd.find(rootWD) + rootWD.size() + 1;
        std::string diff = cwd.substr(ind_diff);

        return CleanPath(diff + '/' + filename);
    }

    std::string CutFileInPath(std::string& pathRelativeToYouLong)
    {
        int i = pathRelativeToYouLong.size() - 1;
        if (IsDirExists(pathRelativeToYouLong))
            return "";
        std::string res;
        while (i >= 0 and pathRelativeToYouLong[i] != '/')
        {
            res = pathRelativeToYouLong[i] + res;
            i--;
        }
        if (i < 0)
            pathRelativeToYouLong = ".";
        else
            pathRelativeToYouLong = pathRelativeToYouLong.substr(0, i);
        return res;
    }

    int CalcHeightDir (const std::string& dir1, const std::string& dir2)
    {
        int countSlash = 0;
        if (dir1.size() >= dir2.size())
        {
            size_t i = dir1.size() - 1;
            while (i >= dir2.size())
            {
                if (dir1[i] == '/')
                    countSlash += 1;
                i--;
            }
        }
        else
            utils::ExitProgramWithMessage(1, "Not implemented");
        return countSlash;
    }

    std::string GetPathRelativeToYourself(const std::string& pathRelativeToYouLongCpy)
    {
        std::string cwd = GetCwd();

        std::string pathRelativeToYouLong = pathRelativeToYouLongCpy;
        std::string fileName = CutFileInPath(pathRelativeToYouLong);
        if (pathRelativeToYouLong.empty())
            pathRelativeToYouLong = ".";

        //std::cout << "FROM: " << pathRelativeToYouLongCpy << ", newpath: " << pathRelativeToYouLong << ", filename: " << fileName << "\n";

        /// Create dirs if not already here
        std::string dirToRemove = CreateDirectoriesAboveFileReturnFirstToDelete(CleanPath(pathRelativeToYouLongCpy));

        utils::ChangeDirWrapper(pathRelativeToYouLong);

        std::string cwd2 = GetCwd();

        utils::ChangeDirWrapper(cwd);
        //std::cout << cwd << ' ' << cwd2 << "\n\n";

        /// Remove potential dirs that were created previously
        if (not dirToRemove.empty())
        {
            std::string command = "rm -rf " + dirToRemove;
            int pid = system(command.c_str());
            int status;
            waitpid(pid, &status, 0);
        }

        if (cwd.find(cwd2) == std::string::npos and cwd2.find(cwd) == std::string::npos)
            return pathRelativeToYouLongCpy;
        else if (cwd == cwd2)
            return fileName;
        else if (cwd2.size() > cwd.size())
        {
            size_t ind_diff = cwd2.find(cwd) + cwd.size() + 1;
            std::string res = cwd2.substr(ind_diff);
            return utils::CleanPath(res + "/" + fileName);
        }
        else
        {
            int diffHeight = CalcHeightDir(cwd, cwd2);
            std::string res;
            while (diffHeight > 0)
            {
                res += "../";
                diffHeight--;
            }
            return utils::CleanPath(res + fileName);
        }
        return "";
    }

    std::map<std::string, std::string> GetEntriesFromIndex (const std::string& input)
    {
        std::map<std::string, std::string> res;
        std::string filePath, hash;
        bool hit_middle = false;
        for (size_t i = 0; i < input.size(); i++)
        {
            if (input[i] == '\n')
            {
                res.insert({filePath, hash});

                hit_middle = false;
                filePath.clear();
                hash.clear();
            }
            else if (input[i] == ' ')
            {
                hit_middle = true;
            }
            else
            {
                if (not hit_middle)
                    hash += input[i];
                else
                    filePath += input[i];
            }
        }
        return res;
    }

    std::vector<std::string> GetEntriesFromIndexAsList (const std::string& input)
    {
        std::map<std::string, std::string> map = GetEntriesFromIndex(input);
        std::vector<std::string> res;
        res.reserve(map.size());
        for (const auto& elm : map)
        {
            res.push_back(elm.first);
        }
        return res;
    }

    std::map<std::string, std::string> ReadIndexAndGetEntries ()
    {
        std::string indexContents = utils::ReadFile(PathToIndex());
        std::string decompressed;
        if (not indexContents.empty())
            decompressed = utils::DecompressString(indexContents);
        return GetEntriesFromIndex(decompressed);
    }

    std::vector<std::string> ReadIndexAndGetEntriesIndexAsList ()
    {
        std::string indexContents = utils::ReadFile(PathToIndex());
        std::string decompressed;
        if (not indexContents.empty())
            decompressed = utils::DecompressString(indexContents);
        return GetEntriesFromIndexAsList(decompressed);
    }

    void IterateDir (const std::string& path, std::vector<std::string>& files)
    {
        DIR *dir = opendir(path.c_str());
        if (dir == nullptr)
            return;
        else
        {
            struct dirent *ent;
            while ((ent = readdir(dir)))
            {
                std::string tmp = ent->d_name;
                if (tmp == "." or tmp == "..")
                    continue;

                std::string new_path;
                if (path[path.size() - 1] == '/')
                    new_path = path + ent->d_name;
                else
                    new_path = path + "/" + ent->d_name;

                if (IsFileExcluded(new_path))
                    continue;

                if (IsFileExists(new_path))
                    files.push_back(CleanPath(new_path));

                DIR *tstdir = opendir(new_path.c_str());
                if (tstdir != nullptr)
                {
                    IterateDir(new_path, files);
                    closedir(tstdir);
                }
            }
        }
        closedir(dir);
    }

    bool IsFileExcluded (const std::string& path)
    {
        std::string db_pattern = "*" + g_DB_FILE + "*";
        if (not fnmatch(db_pattern.c_str(), path.c_str(), 0))
            return true;

        for (const auto& pattern : g_myGitIgnorePatterns)
        {
            std::string patternForNow = "*" + pattern + "*";
            if (not fnmatch(patternForNow.c_str(), path.c_str(), 0))
                return true;
        }

        return false;
    }

    std::vector<std::string> ReadMyGitIgnorePatterns(const std::string& contents)
    {
        std::vector<std::string> res;
        std::string cur;
        for (size_t i = 0; i < contents.size(); i++)
        {
            if (contents[i] == '\n' or i == contents.size() - 1)
            {
                if (i == contents.size() - 1 and contents[i] != '\n')
                    cur += contents[i];
                res.push_back(cur);
                cur.clear();
            }
            else
            {
                cur += contents[i];
            }
        }
        return res;
    }

    std::vector<std::string> GetWorkingDirectoryFiles ()
    {
        std::vector<std::string> files;
        IterateDir(utils::PathToRootRepo(), files);
        return files;
    }

    std::vector<std::string> GetCurrentDirectoryFiles (const std::string& pathToDir)
    {
        std::vector<std::string> files;
        IterateDir(pathToDir, files);
        return files;
    }

    std::string ReadHEAD()
    {
        std::string contents = ReadFile(PathToHEAD());
        return contents;
    }

    void GetWorkDirFileStatus (std::map<std::string, std::string>& added, std::map<std::string, std::string>& deleted,
                               std::map<std::string, std::string>& modified)
    {
        /// Read index files
        std::vector<std::string> indexEntries = utils::ReadIndexAndGetEntriesIndexAsList();
        /// Read working directory files
        std::vector<std::string> workDirEntries = utils::GetWorkingDirectoryFiles();
        std::vector<std::string> workDirEntriesFromActualPos = workDirEntries;
        for (size_t i = 0; i < workDirEntries.size(); i++)
        {
            workDirEntriesFromActualPos[i] = utils::GetPathRelativeToYourself(workDirEntries[i]);
            workDirEntries[i] = utils::GetPathRelativeToDotMyGit(workDirEntries[i]);
        }

        for (size_t i = 0; i < workDirEntries.size(); i++)
        {
            std::string wdFile = workDirEntries[i];
            std::string wdFileFromActualPos = workDirEntriesFromActualPos[i];
            if (std::find(indexEntries.begin(), indexEntries.end(), wdFile) != indexEntries.end() and not utils::IsFileExcluded(wdFile))
            {
                objects::Blob blob = objects::Blob(wdFile, wdFileFromActualPos);
                std::string hash = blob.ToHash();
                std::string blobPath = utils::PathToObjectFile(hash);
                if (not utils::IsFileExists(blobPath))
                {
                    modified.insert({ wdFile, wdFileFromActualPos });
                }
            }
            /// File not in index, there was added
            else
            {
                if (not utils::IsFileExcluded(wdFile))
                {
                    added.insert({ wdFile, wdFileFromActualPos });
                }
            }
        }

        for (const auto& indexEntry : indexEntries)
        {
            if (std::find(workDirEntries.begin(), workDirEntries.end(), indexEntry) == workDirEntries.end())
            {
                if (not utils::IsFileExcluded(indexEntry))
                {
                    deleted.insert({indexEntry, utils::GetPathRelativeToYourself(utils::CleanPath(utils::AppendPathToRootRepo(indexEntry))) });
                }
            }
        }
    }

    bool IsWorkingDirectoryClean ()
    {
        std::map<std::string, std::string> added, deleted, modified;
        utils::GetWorkDirFileStatus(added, deleted, modified);
        return added.empty() and deleted.empty() and modified.empty();
    }

    std::vector<std::string> ListEntriesInDirOneLayer (const std::string& path)
    {
        std::vector<std::string> res;
        DIR *dir = opendir(path.c_str());
        struct dirent *ent;
        while ((ent = readdir(dir)))
        {
            std::string entName = ent->d_name;
            if (entName == "." or entName == "..")
                continue;
            res.push_back(entName);
        }
        closedir(dir);
        return res;
    }

    bool IsDirEmpty (const std::string& path)
    {
        return ListEntriesInDirOneLayer(path).empty();
    }

    void DeleteDirectoryIfEmpty (const std::string& pathFileFromDotMyGit)
    {
        std::string dummy;
        for (size_t i = 0; i < pathFileFromDotMyGit.size(); i++)
        {
            dummy += pathFileFromDotMyGit[i];
            if (utils::IsDirExists(dummy))
            {
                if (utils::IsDirEmpty(dummy))
                {
                    //std::cout << "RM " << dummy << '\n';
                    std::string command = "rm -rf " + dummy;
                    int pid = system(command.c_str());
                    int status;
                    waitpid(pid, &status, 0);
                    break;
                }
            }
        }
    }

    std::string CreateDirectoriesAboveFileReturnFirstToDelete (const std::string& pathFileFromDotMyGit)
    {
        std::string dummy;
        std::string todel;
        for (size_t i = 0; i < pathFileFromDotMyGit.size(); i++)
        {
            dummy += pathFileFromDotMyGit[i];
            if (dummy[i] == '/')
            {
                if (not utils::IsDirExists(dummy))
                {
                    if (todel.empty())
                        todel = dummy;
                    utils::CreateDir(dummy);
                }
            }
        }
        return todel;
    }
}