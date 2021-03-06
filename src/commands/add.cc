#include "commands/commands.hh"

#include "objects/blob.hh"

#include "utils/wrappers.hh"
#include "utils/get_paths.hh"
#include "utils/zlib.hh"
#include "utils/utils.hh"

namespace mygit
{
    void add(int argc, char *argv[])
    {
        /// Create opt object
        auto opt = options::AddOptions(argc, argv);

        for (const auto& path : opt.pathArguments)
        {
            /// Handle git add of deleted files
            std::string pathRelativeToDotMyGit = utils::CleanPath(utils::GetPathRelativeToDotMyGit(path));
            if (pathRelativeToDotMyGit == "." or pathRelativeToDotMyGit == "./")
                pathRelativeToDotMyGit = "";
            std::vector<std::string> indexEntries = utils::ReadIndexAndGetEntriesIndexAsList();
            std::vector<std::string> removeFromIndex;
            for (const auto& indEntry : indexEntries)
            {
                if (not utils::IsFileExists(utils::AppendPathToRootRepo(indEntry))
                and (indEntry == pathRelativeToDotMyGit or indEntry.find(pathRelativeToDotMyGit) != std::string::npos))
                {
                    removeFromIndex.push_back(indEntry);
                }
            }

            /// Case where 'path' leads to a directory
            if (utils::IsDirExists(path) or not removeFromIndex.empty())
            {
                /// Retreive all files from this directory
                std::vector<std::string> entries = utils::GetCurrentDirectoryFiles(path);
                UpdateIndexMultipleFiles(entries, opt, removeFromIndex);
            }
            /// Case where 'path' lads to a regular file
            else if (utils::IsFileExists(path) or not removeFromIndex.empty())
            {
                UpdateIndex(path, opt, removeFromIndex);
            }
            /// Wrongly formatted argument case
            else
            {
                utils::ExitProgramWithMessage(1, "You need to specify a valid parameter to the 'add' command.");
            }
        }
    }

    void UpdateIndex(const std::string& pathToFile, const options::AddOptions& opt, const std::vector<std::string>& removeFromIndex)
    {
        std::vector<std::string> paths = { pathToFile };
        UpdateIndexMultipleFiles(paths, opt, removeFromIndex);
    }

    void UpdateIndexMultipleFiles(const std::vector<std::string>& pathsToFiles, const options::AddOptions& opt, const std::vector<std::string>& removeFromIndex)
    {
        std::map<std::string, std::string> hashesAndPaths;
        for (const auto& pathToFile : pathsToFiles)
        {
            /// Get path to file relative to .mygit file
            std::string pathFileFromDotMyGit = utils::GetPathRelativeToDotMyGit(pathToFile);
            if (not opt.force and utils::IsFileExcluded(pathFileFromDotMyGit))
            {
                std::cout << "File \'" << pathFileFromDotMyGit << "\' is supposed to be ignored, therefore was not added.\n";
                continue;
            }
            /// Create blob (hash + compressed file) in .mygit/objects
            std::string hash = objects::CreateBlob(pathFileFromDotMyGit);

            hashesAndPaths.insert({pathFileFromDotMyGit, hash});
        }

        /// Update Index file
        utils::AddOrRemoveElementsInIndex(hashesAndPaths, removeFromIndex);
    }
}