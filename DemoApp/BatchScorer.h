#pragma once

// Loads and scores each DICOM set in a file list, one case at a time and headless
// (no viewer dialog), via its own CIQVManager - so only one case's images are ever
// in memory at once, regardless of how many cases are in the list.
class CBatchScorer
{
public:
	CBatchScorer();
	~CBatchScorer();

	// list holds one sample filename per case, e.g. as produced by
	// CMyWindows::ListSampleFilesInDirTree. Returns the number of cases scored.
	int Run(const class CFilesList& list);
};
