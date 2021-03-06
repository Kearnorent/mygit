#include "objects/blob.hh"

#include "utils/get_paths.hh"
#include "utils/sha1.hh"
#include "utils/zlib.hh"
#include "utils/utils.hh"

namespace objects
{
    Blob::Blob(const std::string& pathFileFromDotMyGit, const std::string& fullPath)
    {
        _pathFileFromDotMyGit = pathFileFromDotMyGit;
        _contentsFile = utils::ReadFile(fullPath);
    }

    std::string Blob::GetContentsFile() const
    {
        return _contentsFile;
    }

    std::string Blob::ToString() const
    {
        return "blob;" + _pathFileFromDotMyGit + ';' + _contentsFile + ';';
    }

    std::string Blob::ToHash()
    {
        /// The Blob hash is made out of the ToString() result
        std::string blobString = this->ToString();
        return utils::SHA1_Wrapper(blobString);
    }

    std::string Blob::GetPathFromDotMyGit() const
    {
        return _pathFileFromDotMyGit;
    }

    void SetupBlob(const objects::Blob& blob, const std::string& hash)
    {
        /// Store the blob
        std::string blobDirName = utils::PathToObjectDir(hash);
        if (not utils::IsDirExists(blobDirName))
            utils::CreateDir(blobDirName);

        /// Fill in the contents of the blob (compressed file)
        std::string blobFilePath = utils::PathToObjectFile(hash);

        /// Sets the blob metadata
        std::string blob_data = "type=blob\nfilepath=" + blob.GetPathFromDotMyGit() + "\nsize=" + std::to_string(blob.GetContentsFile().size()) + "\n\n" + blob.GetContentsFile();

        utils::WriteFile(blobFilePath, utils::CompressString(blob_data));
    }

    std::string CreateBlob (const std::string& pathFileFromDotMyGit)
    {
        /// Create the blob and generate the hash from it
        objects::Blob blob = objects::Blob(pathFileFromDotMyGit, utils::AppendPathToRootRepo(pathFileFromDotMyGit));
        std::string hash = blob.ToHash();

        /// Setup blob (directory and file filling)
        SetupBlob(blob, hash);

        return hash;
    }

    std::string GetTypeBlobDecompressed (const std::string& input)
    {
        for (size_t i = 0; i < input.size(); i++)
        {
            if (input[i] == '=')
            {
                i += 1;
                std::string res;
                while (i < input.size() and input[i] != '\n')
                {
                    res += input[i++];
                }
                return res;
            }
        }
        return "";
    }

    std::string GetContentBlobDecompressed (const std::string& input)
    {
        for (size_t i = 0; i < input.size(); i++)
        {
            if (i + 1 < input.size() and input[i] == '\n' and input[i + 1] == '\n')
            {
                i += 2;
                std::string res;
                while (i < input.size())
                {
                    res += input[i++];
                }
                return res;
            }
        }
        return "";
    }
}