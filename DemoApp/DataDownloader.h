#pragma once

// "Utils > Download Data" - fetches DICOM image sets from a network (or any) source tree into
// the local gConfig.msDataRoot, mirroring the source's directory structure so each set's origin
// stays identifiable. Only sets found under a directory named gConfig.msDownloadDirNameFilter
// (e.g. "Water", case-insensitive) are fetched - most of the source tree is other data we don't want.
class CDataDownloader
{
public:
	CDataDownloader();
	~CDataDownloader();

	// Scans zRootDir for gConfig.msDownloadDirNameFilter directories, and under each of those
	// for DICOM image sets, copying every one found. Returns the number of sets copied.
	int DownloadFromRoot(const char* zRootDir);

private:
	// Reports zText both to the GUI's status line and to the console
	void MyPrintStatus(const char* zText);

	// Copies the DICOM set whose sample file is sSampleFile into its corresponding location
	// under sDestRoot, preserving the path between sRoot and the set's own directory.
	void CopyOneSet(const CString& sRoot, const CString& sDestRoot, const CString& sSampleFile, int& onCopied);
};
