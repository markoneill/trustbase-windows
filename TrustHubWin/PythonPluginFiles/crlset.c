#include <stdio.h>
#include "crlset.h"

// force gcc to link in go runtime (may be a better solution than this)
void dummy() {
    getCurrentVersionFromPython();
	fetchFromPython();
	char* p0 = "test";
	char* p1 = "test";
	char* p2 = "test";

	dumpFromPython(p0, p1, p2);
	dumpSPKIFromPython(p0);
}

int main() {

}