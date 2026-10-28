// forcallerhopfix/rankgo — UPWARD-2HOP F2: the seed method (memStore.Put) is reached through an interface, so its only first-hop caller is a by-name match
package store

import "errors"

type Store interface {
	Put(key string) error
}

type memStore struct{ keys []string }

func (m *memStore) Put(key string) error {
	for _, k := range m.keys {
		if k == key {
			return errors.New("duplicate key error")
		}
	}
	m.keys = append(m.keys, key)
	return nil
}

type diskStore struct{}

func (d *diskStore) Put(key string) error { return nil }

func saveAll(s Store, keys []string) error {
	for _, k := range keys {
		if err := s.Put(k); err != nil {
			return err
		}
	}
	return nil
}

func Import(keys []string) error {
	return saveAll(&memStore{}, keys)
}

func Sync(keys []string) error {
	return saveAll(&diskStore{}, keys)
}
