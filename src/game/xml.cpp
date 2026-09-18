// game/xml: XmlSimple, a minimal tag reader / writer over C strings (the tool-side "XSDSchemaSof"
// node files: <Node> blocks of <name>value</name> elements); string search only, no parser.
#include "types.h"
#include "xml.h"

extern "C" {
unsigned int strlen(const char* s);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, unsigned int n);
char* strcat(char* dst, const char* src);
char* strstr(const char* s, const char* sub);
int sprintf(char* buf, const char* fmt, ...);
}

// Finds "<tag" in `src`; *out = its position. 0 when absent.
int XmlSimple::GetXmlStart(char** out, const char* src, const char* tag)
{
    char buf[256];

    strcpy(buf, "<");
    strcat(buf, tag);
    *out = strstr(src, buf);
    if (*out == NULL) {
        return 0;
    }
    return 1;
}

// The next "<tag" after position `src`.
int XmlSimple::GetXmlNext(char** out, const char* src, const char* tag)
{
    return GetXmlStart(out, src + 1, tag);
}

// Copies the text between "<tag>" and "</tag>" into `out`. 0 when either is missing.
int XmlSimple::GetXmlElem(char* out, const char* src, const char* tag)
{
    char start[256];
    char end[256];
    char* p;
    char* q;
    int len;

    strcpy(start, "<");
    strcat(start, tag);
    strcat(start, ">");
    p = strstr(src, start);
    if (p == NULL) {
        return 0;
    }
    p += strlen(start);
    strcpy(end, "</");
    strcat(end, tag);
    strcat(end, ">");
    q = strstr(src, end);
    if (q == NULL) {
        return 0;
    }
    len = q - p;
    strncpy(out, p, len);
    out[len] = '\0';
    return 1;
}

// Writes the document opening tag; *size grows by its length.
int XmlSimple::SetXmlStart(int* size, char* buf)
{
    strcpy(buf, "<XSDSchemaSof xmlns=\"http://tempuri.org/XSDSchemaSof.xsd\">\n");
    *size += strlen(buf);
    return 1;
}

// Writes the document closing tag.
int XmlSimple::SetXmlEnd(int* size, char* buf)
{
    strcpy(buf, "</XSDSchemaSof>\n");
    *size += strlen(buf);
    return 1;
}

// Writes "<Node>".
int XmlSimple::SetXmlElemStart(int* size, char* buf)
{
    strcpy(buf, "\t<Node xmlns=\"\">\n");
    *size += strlen(buf);
    return 1;
}

// Writes "</Node>".
int XmlSimple::SetXmlElemEnd(int* size, char* buf)
{
    strcpy(buf, "\t</Node>\n");
    *size += strlen(buf);
    return 1;
}

// Writes "<name>value</name>".
int XmlSimple::SetXmlElem(int* size, char* buf, const char* name, const char* value)
{
    char tmp[256];

    sprintf(tmp, "\t\t<%s>%s</%s>\n", name, value, name);
    strcpy(buf, tmp);
    *size += strlen(buf);
    return 1;
}
