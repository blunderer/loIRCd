
SRC = loIRCd.c
OBJ = loIRCd.o

CFLAGS = -g -O3 -Wall -Wextra -I.
LDFLAGS = -lpthread

TARGET = loIRCd

# rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

clean:
	$(RM) $(OBJ)
	$(RM) $(TARGET)

