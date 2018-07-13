#ifndef _MULTIPART_PARSER_H_
#define _MULTIPART_PARSER_H_

#include <sys/types.h>
#include <string>
#include <stdexcept>
#include <cstring>
#include <vector>

class MultipartParser {
public:
	typedef void (*Callback)(const char *buffer, size_t start, size_t end, void *userData);
	
private:
	static const char CR     = 13;
	static const char LF     = 10;
	static const char SPACE  = 32;
	static const char HYPHEN = 45;
	static const char COLON  = 58;
	static const size_t UNMARKED = (size_t) -1;
	
	enum State {
		ERROR,
		START,
		START_BOUNDARY,
		HEADER_START,
		// Checking for possible duplicate boundary.
		DUPLICATE_BOUNDARY_START,
		// Checking for possible duplicate boundary preceded by an empty line.
		CRLF_DUPLICATE_BOUNDARY_START,
		HEADER_FIELD_START,
		HEADER_FIELD,
		HEADER_VALUE_START,
		HEADER_VALUE,
		HEADER_VALUE_ALMOST_DONE,
		HEADERS_ALMOST_DONE,
		PART_DATA_START,
		PART_DATA,
		PART_END,
		END
	};
	
	enum Flags {
		PART_BOUNDARY = 1,
		LAST_BOUNDARY = 2
	};
	
	std::string boundary;
	bool boundaryIndex[256];
	std::vector<char> lookbehind;
	std::string duplicateBoundary;
	// Buffer from which to replay bytes of what was a potential duplicate boundary
	std::vector<char> replayBuffer;
	State state;
	int flags;
	size_t index;
	size_t headerFieldMark;
	size_t headerValueMark;
	size_t partDataMark;
	const char *errorReason;
	
	void resetCallbacks() {
		onPartBegin   = NULL;
		onHeaderField = NULL;
		onHeaderValue = NULL;
		onHeaderEnd   = NULL;
		onHeadersEnd  = NULL;
		onPartData    = NULL;
		onPartEnd     = NULL;
		onEnd         = NULL;
		userData      = NULL;
	}
	
	void indexBoundary() {
		const char *current;
		const char *end = boundary.c_str() + boundary.size();
		
		memset(boundaryIndex, 0, sizeof(boundaryIndex));
		
		for (current = boundary.c_str(); current < end; current++) {
			boundaryIndex[(unsigned char) *current] = true;
		}
	}
	
	void callback(Callback cb, const char *buffer = NULL, size_t start = UNMARKED,
		size_t end = UNMARKED, bool allowEmpty = false)
	{
		if (start != UNMARKED && start == end && !allowEmpty) {
			return;
		}
		if (cb != NULL) {
			cb(buffer, start, end, userData);
		}
	}
	
	void dataCallback(Callback cb, size_t &mark, const char *buffer, size_t i, size_t bufferLen,
		bool clear, bool allowEmpty = false)
	{
		if (mark == UNMARKED) {
			return;
		}
		
		if (!clear) {
			callback(cb, buffer, mark, bufferLen, allowEmpty);
			mark = 0;
		} else {
			callback(cb, buffer, mark, i, allowEmpty);
			mark = UNMARKED;
		}
	}
	
	char lower(char c) const {
		return c | 0x20;
	}
	
	inline bool isBoundaryChar(char c) const {
		return boundaryIndex[(unsigned char) c];
	}
	
	bool isHeaderFieldCharacter(char c) const {
		return (c >= 'a' && c <= 'z')
			|| (c >= 'A' && c <= 'Z')
			|| c == HYPHEN;
	}
	
	void setError(const char *message) {
		state = ERROR;
		errorReason = message;
	}
	
	void processPartData(size_t &prevIndex, size_t &index, const char *buffer,
		size_t len, size_t boundaryEnd, size_t &i, char c, State &state, int &flags)
	{
		prevIndex = index;
		
		if (index == 0) {
			// boyer-moore derived algorithm to safely skip non-boundary data
			while (i + boundary.size() <= len) {
				if (isBoundaryChar(buffer[i + boundaryEnd])) {
					break;
				}
				
				i += boundary.size();
			}
			if (i == len) {
				return;
			}
			c = buffer[i];
		}
		
		if (index < boundary.size()) {
			if (boundary[index] == c) {
				if (index == 0) {
					dataCallback(onPartData, partDataMark, buffer, i, len, true);
				}
				index++;
			} else {
				index = 0;
			}
		} else if (index == boundary.size()) {
			index++;
			if (c == CR) {
				// CR = part boundary
				flags |= PART_BOUNDARY;
			} else if (c == HYPHEN) {
				// HYPHEN = end boundary
				flags |= LAST_BOUNDARY;
			} else {
				index = 0;
			}
		} else if (index - 1 == boundary.size()) {
			if (flags & PART_BOUNDARY) {
				index = 0;
				if (c == LF) {
					// unset the PART_BOUNDARY flag
					flags &= ~PART_BOUNDARY;
					callback(onPartEnd);
					callback(onPartBegin);
					state = HEADER_START;
					return;
				}
			} else if (flags & LAST_BOUNDARY) {
				if (c == HYPHEN) {
					callback(onPartEnd);
					callback(onEnd);
					state = END;
				} else {
					index = 0;
				}
			} else {
				index = 0;
			}
		} else if (index - 2 == boundary.size()) {
			if (c == CR) {
				index++;
			} else {
				index = 0;
			}
		} else if (index - boundary.size() == 3) {
			index = 0;
			if (c == LF) {
				callback(onPartEnd);
				callback(onEnd);
				state = END;
				return;
			}
		}
		
		if (index > 0) {
			// when matching a possible boundary, keep a lookbehind reference
			// in case it turns out to be a false lead
			if (index - 1 >= lookbehind.size()) {
				setError("Parser bug: index overflows lookbehind buffer. "
					"Please send bug report with input file attached.");
				throw std::out_of_range("index overflows lookbehind buffer");
			} else if (index < 1) {
				setError("Parser bug: index underflows lookbehind buffer. "
					"Please send bug report with input file attached.");
				throw std::out_of_range("index underflows lookbehind buffer");
			}
			lookbehind[index - 1] = c;
		} else if (prevIndex > 0) {
			// if our boundary turned out to be rubbish, the captured lookbehind
			// belongs to partData
			callback(onPartData, lookbehind.data(), 0, prevIndex);
			prevIndex = 0;
			partDataMark = i;
			
			// reconsider the current character even so it interrupted the sequence
			// it could be the beginning of a new sequence
			i--;
		}
	}
	
public:
	Callback onPartBegin;
	Callback onHeaderField;
	Callback onHeaderValue;
	Callback onHeaderEnd;
	Callback onHeadersEnd;
	Callback onPartData;
	Callback onPartEnd;
	Callback onEnd;
	void *userData;
	
	MultipartParser() {
		resetCallbacks();
		reset();
	}
	
	MultipartParser(const std::string &boundary) {
		resetCallbacks();
		setBoundary(boundary);
	}
	
	void reset() {
		lookbehind.clear();
		state = ERROR;
		boundary.clear();
		flags = 0;
		index = 0;
		headerFieldMark = UNMARKED;
		headerValueMark = UNMARKED;
		partDataMark    = UNMARKED;
		errorReason     = "Parser uninitialized.";
	}
	
	void setBoundary(const std::string &boundary) {
		reset();
		this->boundary = "\r\n--" + boundary;
		indexBoundary();
		lookbehind.resize(boundary.size() + 8);
		duplicateBoundary = this->boundary + "\r\n";
		replayBuffer.resize(duplicateBoundary.size() + 1);
		state = START;
		errorReason = "No error.";
	}
	
	size_t feed(const char *buffer, size_t len) {
		if (state == ERROR || len == 0) {
			return 0;
		}
		
		State state         = this->state;
		int flags           = this->flags;
		size_t prevIndex    = this->index;
		size_t index        = this->index;
		size_t boundaryEnd  = boundary.size() - 1;
		size_t i;
		char c, cl;
		// 'i' value to re-instate when done replaying content of replayBuffer.
		size_t saveI = 0;
		// 'len' value to re-instate when done replaying content of replayBuffer.
		size_t saveLen = 0;
		// 'buffer' value to re-instate when done replaying content of replayBuffer.
		const char* saveBuffer = nullptr;

		i = 0;
		while (i < len) {

			c = buffer[i];
			
			switch (state) {
			case ERROR:
				return i;
			case START:
				index = 0;
				state = START_BOUNDARY;
			case START_BOUNDARY:
				if (index == boundary.size() - 2) {
					if (c != CR) {
						setError("Malformed. Expected CR after boundary.");
						return i;
					}
					index++;
					break;
				} else if (index - 1 == boundary.size() - 2) {
					if (c != LF) {
						setError("Malformed. Expected LF after boundary CR.");
						return i;
					}
					index = 0;
					callback(onPartBegin);
					state = HEADER_START;
					break;
				}
				if (c != boundary[index + 2]) {
					setError("Malformed. Found different boundary data than the given one.");
					return i;
				}
				index++;
				break;
			case DUPLICATE_BOUNDARY_START:
			case CRLF_DUPLICATE_BOUNDARY_START: {
				size_t offset = (state == DUPLICATE_BOUNDARY_START ? 2 : 0);
				size_t duplicateIndex = index + offset;
				if (c != duplicateBoundary[duplicateIndex]) {
					// Char does not match duplicate boundary.  Abort the check and replay the processed bytes.
					saveBuffer = buffer;
					saveLen = len;
					saveI = i;
					std::copy(
							duplicateBoundary.c_str(), duplicateBoundary.c_str() + duplicateIndex + 1,
							replayBuffer.begin());
					replayBuffer[duplicateIndex + 1] = c;
					buffer = &replayBuffer[0];
					len = duplicateIndex + 2;
					i = 0;
					state = HEADER_FIELD;
					headerFieldMark = i;
					index = 0;
				} else if (duplicateIndex + 1 < duplicateBoundary.size()) {
					// Char matches, continue verifying whether this is a duplicate boundary.
					index++;
				} else {
					// Duplicate boundary detected.  Skip over it.
					state = HEADER_START;
				}
				break;
			}
			case HEADER_START:
				headerFieldMark = i;
				if (c == HYPHEN) {
					state = DUPLICATE_BOUNDARY_START;
					index = 1;
					break;
				} else if (c == CR) {
					state = CRLF_DUPLICATE_BOUNDARY_START;
					index = 1;
					break;
				}
				// Fall through to HEADER_FIELD_START
			case HEADER_FIELD_START:
				state = HEADER_FIELD;
				headerFieldMark = i;
				index = 0;
			case HEADER_FIELD:
				if (c == CR) {
					headerFieldMark = UNMARKED;
					state = HEADERS_ALMOST_DONE;
					break;
				}

				index++;
				if (c == HYPHEN) {
					break;
				}

				if (c == COLON) {
					if (index == 1) {
						// empty header field
						setError("Malformed first header name character.");
						return i;
					}
					dataCallback(onHeaderField, headerFieldMark, buffer, i, len, true);
					state = HEADER_VALUE_START;
					break;
				}

				cl = lower(c);
				if (cl < 'a' || cl > 'z') {
					setError("Malformed header name.");
					return i;
				}
				break;
			case HEADER_VALUE_START:
				if (c == SPACE) {
					break;
				}
				
				headerValueMark = i;
				state = HEADER_VALUE;
			case HEADER_VALUE:
				if (c == CR) {
					dataCallback(onHeaderValue, headerValueMark, buffer, i, len, true, true);
					callback(onHeaderEnd);
					state = HEADER_VALUE_ALMOST_DONE;
				}
				break;
			case HEADER_VALUE_ALMOST_DONE:
				if (c != LF) {
					setError("Malformed header value: LF expected after CR");
					return i;
				}
				
				state = HEADER_FIELD_START;
				break;
			case HEADERS_ALMOST_DONE:
				if (c != LF) {
					setError("Malformed header ending: LF expected after CR");
					return i;
				}
				
				callback(onHeadersEnd);
				state = PART_DATA_START;
				break;
			case PART_DATA_START:
				state = PART_DATA;
				partDataMark = i;
			case PART_DATA:
				processPartData(prevIndex, index, buffer, len, boundaryEnd, i, c, state, flags);
				break;
			default:
				return i;
			}

			if (++i >= len) {
				dataCallback(onHeaderField, headerFieldMark, buffer, i, len, false);
				dataCallback(onHeaderValue, headerValueMark, buffer, i, len, false);
				dataCallback(onPartData, partDataMark, buffer, i, len, false);
				// If we have exhausted replayBuffer, resume parsing the original input buffer.
				if (saveBuffer) {
				    buffer = saveBuffer;
					len = saveLen;
					i = saveI;
					saveBuffer = nullptr;
				}
			}
		}
		
		this->index = index;
		this->state = state;
		this->flags = flags;
		
		return len;
	}
	
	bool succeeded() const {
		return state == END;
	}
	
	bool hasError() const {
		return state == ERROR;
	}
	
	bool stopped() const {
		return state == ERROR || state == END;
	}
	
	const char *getErrorMessage() const {
		return errorReason;
	}
};

#endif /* _MULTIPART_PARSER_H_ */
