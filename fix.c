```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *fp = fopen("test.pkg", "w"); /* Create test file */
    if (!fp) {
        fprintf(stderr, "Failed to create test file\n");
        return 1;
    }
    fclose(fp);
    
    char *cmd = malloc(100);
    if (!cmd) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    sprintf(cmd, "mdls -name kMDItemContentType -name kMDItemContentTypeTree \"test.pkg\"");
    system(cmd);
    
    free(cmd);
    remove("test.pkg"); /* Cleanup */
    return 0;
}
```