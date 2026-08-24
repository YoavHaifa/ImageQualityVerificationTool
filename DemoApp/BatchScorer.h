#pragma once

// Loads and scores each DICOM set in a file list, one case at a time and headless
// (no viewer dialog), via its own CIQVManager - so only one case's images are ever
// in memory at once, regardless of how many cases are in the list.
class CBatchScorer
{
public:
	CBatchScorer();
	~CBatchScorer();

	// Scans zRootDir for DICOM sets (one sample file per matching directory, per
	// gConfig.msDicomFilePattern) and scores every one found. Returns the number scored.
	int RunOnDirTree(const char* zRootDir);

	// list holds one sample filename per case, e.g. as produced by
	// CMyWindows::ListSampleFilesInDirTree. Returns the number of cases scored.
	int Run(const class CFilesList& list);

	// This run's own log directory (msLogRoot\<batch root name>), valid after RunOnDirTree() -
	// lets a caller jump straight into Batch Review on the results just produced.
	const CString& GetLogDir(void) const { return msLogDir; }

private:
	// Reports zText both to the GUI's batch status line and to the console
	void MyPrintStatus(const char* zText);

	CString msLogDir;
};
