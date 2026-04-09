// Command windows-io-repro stress-tests concurrent file opens on Windows
// to reproduce the intermittent ERROR_INVALID_PARAMETER (errno 87) seen
// in siso's subninja scan when out/ is served through a container
// bind-mount filter driver.
//
// It creates a large set of small files in -dir, then runs multiple
// NumCPU-parallel scan rounds using two strategies:
//   - stdlib:   os.Open (Go passes FILE_FLAG_BACKUP_SEMANTICS)
//   - nobackup: direct CreateFileW without FILE_FLAG_BACKUP_SEMANTICS
//
// Each errno-87 hit is logged with whether a 5ms retry recovers, so the
// two strategies can be compared head-to-head.
//
//go:build windows

package main

import (
	"errors"
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"sync"
	"sync/atomic"
	"syscall"
	"time"
)

const errnoInvalidParameter = syscall.Errno(87)

// innerSema mirrors siso's fsema: the chunk-read goroutine acquires a
// NumCPU-wide slot before doing its second open.
var innerSema = make(chan struct{}, runtime.NumCPU())

// openStdlib replicates siso's fileParser.readFile: outer os.Open + Stat,
// then a goroutine that does a second os.Open on the same path and ReadAt,
// with the outer handle still held until the goroutine returns.
func openStdlib(path string) error {
	f, err := os.Open(path)
	if err != nil {
		return err
	}
	defer f.Close()
	st, err := f.Stat()
	if err != nil {
		return err
	}
	buf := make([]byte, st.Size())
	errCh := make(chan error, 1)
	go func() {
		innerSema <- struct{}{}
		defer func() { <-innerSema }()
		f2, err := os.Open(path)
		if err != nil {
			errCh <- err
			return
		}
		defer f2.Close()
		_, err = f2.ReadAt(buf, 0)
		errCh <- err
	}()
	return <-errCh
}

func createNoBackup(path string) (syscall.Handle, error) {
	p, err := syscall.UTF16PtrFromString(path)
	if err != nil {
		return syscall.InvalidHandle, err
	}
	h, err := syscall.CreateFile(
		p,
		syscall.GENERIC_READ,
		syscall.FILE_SHARE_READ|syscall.FILE_SHARE_WRITE,
		nil,
		syscall.OPEN_EXISTING,
		syscall.FILE_ATTRIBUTE_NORMAL,
		0,
	)
	if err != nil {
		return syscall.InvalidHandle, &os.PathError{Op: "CreateFile", Path: path, Err: err}
	}
	return h, nil
}

// openNoBackup mirrors openStdlib's double-open shape but uses
// CreateFileW without FILE_FLAG_BACKUP_SEMANTICS for both opens.
func openNoBackup(path string) error {
	h1, err := createNoBackup(path)
	if err != nil {
		return err
	}
	defer syscall.CloseHandle(h1)
	errCh := make(chan error, 1)
	go func() {
		innerSema <- struct{}{}
		defer func() { <-innerSema }()
		h2, err := createNoBackup(path)
		if err != nil {
			errCh <- err
			return
		}
		errCh <- syscall.CloseHandle(h2)
	}()
	return <-errCh
}

type result struct {
	opens      atomic.Int64
	err87      atomic.Int64
	err87Retry atomic.Int64 // errno 87 that cleared on immediate retry
	otherErr   atomic.Int64
}

func scan(name string, open func(string) error, files []string, workers int, r *result) time.Duration {
	start := time.Now()
	ch := make(chan string, workers)
	var wg sync.WaitGroup
	for range workers {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for path := range ch {
				r.opens.Add(1)
				err := open(path)
				if err == nil {
					continue
				}
				if errors.Is(err, errnoInvalidParameter) {
					r.err87.Add(1)
					time.Sleep(5 * time.Millisecond)
					if open(path) == nil {
						r.err87Retry.Add(1)
						fmt.Printf("[%s] errno87 %s — recovered on retry\n", name, path)
					} else {
						fmt.Printf("[%s] errno87 %s — retry FAILED\n", name, path)
					}
				} else {
					r.otherErr.Add(1)
					fmt.Printf("[%s] other error %s: %v\n", name, path, err)
				}
			}
		}()
	}
	for _, f := range files {
		ch <- f
	}
	close(ch)
	wg.Wait()
	return time.Since(start)
}

func main() {
	dir := flag.String("dir", "repro-files", "directory to create/scan test files in")
	nfiles := flag.Int("files", 50000, "number of test files to create")
	rounds := flag.Int("rounds", 10, "scan rounds per strategy")
	workers := flag.Int("workers", runtime.NumCPU(), "concurrent openers per scan")
	writeOnly := flag.Bool("write-only", false, "create files then exit (for cross-process write→scan mode)")
	skipCreate := flag.Bool("skip-create", false, "scan existing files in -dir instead of creating new ones")
	flag.Parse()

	fmt.Printf("GOOS=%s GOARCH=%s NumCPU=%d GOMAXPROCS=%d workers=%d\n",
		runtime.GOOS, runtime.GOARCH, runtime.NumCPU(), runtime.GOMAXPROCS(0), *workers)

	if err := os.MkdirAll(*dir, 0o755); err != nil {
		fmt.Fprintln(os.Stderr, "mkdir:", err)
		os.Exit(1)
	}
	abs, _ := filepath.Abs(*dir)
	fmt.Printf("test dir: %s\n", abs)

	var files []string
	if *skipCreate {
		err := filepath.WalkDir(*dir, func(p string, d os.DirEntry, err error) error {
			if err == nil && !d.IsDir() && filepath.Ext(p) == ".ninja" {
				files = append(files, p)
			}
			return nil
		})
		if err != nil {
			fmt.Fprintln(os.Stderr, "walk:", err)
			os.Exit(1)
		}
		fmt.Printf("scanning %d existing files\n", len(files))
	} else {
		fmt.Printf("creating %d files...\n", *nfiles)
		payload := make([]byte, 256)
		files = make([]string, 0, *nfiles)
		for i := range *nfiles {
			sub := filepath.Join(*dir, fmt.Sprintf("d%03d", i%512))
			if i < 512 {
				_ = os.MkdirAll(sub, 0o755)
			}
			p := filepath.Join(sub, fmt.Sprintf("f%06d.ninja", i))
			if err := os.WriteFile(p, payload, 0o644); err != nil {
				fmt.Fprintln(os.Stderr, "write:", err)
				os.Exit(1)
			}
			files = append(files, p)
		}
		fmt.Printf("created %d files in %d subdirs\n", len(files), 512)
	}
	if *writeOnly {
		fmt.Println("write-only mode; exiting")
		return
	}

	type strat struct {
		name string
		fn   func(string) error
		r    result
	}
	strats := []*strat{
		{name: "stdlib", fn: openStdlib},
		{name: "nobackup", fn: openNoBackup},
	}

	for round := 1; round <= *rounds; round++ {
		for _, s := range strats {
			d := scan(s.name, s.fn, files, *workers, &s.r)
			fmt.Printf("round %d/%d [%s] %d opens in %s (err87 so far: %d)\n",
				round, *rounds, s.name, len(files), d.Round(time.Millisecond), s.r.err87.Load())
		}
	}

	fmt.Println("\n=== summary ===")
	for _, s := range strats {
		fmt.Printf("[%s] opens=%d err87=%d err87_recovered=%d other_err=%d\n",
			s.name, s.r.opens.Load(), s.r.err87.Load(), s.r.err87Retry.Load(), s.r.otherErr.Load())
	}
	var total int64
	for _, s := range strats {
		total += s.r.err87.Load()
	}
	if total == 0 {
		fmt.Println("no ERROR_INVALID_PARAMETER observed")
	}
}
