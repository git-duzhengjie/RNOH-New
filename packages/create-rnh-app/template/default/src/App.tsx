/**
 * {{PROJECT_NAME}} - App entry point
 */

import React from 'react';
import { FlatList, StyleSheet, Text, View } from 'react-native';

interface DemoItem {
  id: string;
  title: string;
  subtitle: string;
}

const DEMO_ITEMS: DemoItem[] = [
  { id: '1', title: '🚀 RNOH-New', subtitle: 'React Native for HarmonyOS' },
  { id: '2', title: '⚙️ TurboModules', subtitle: 'Native modules bridge' },
  { id: '3', title: '🎨 Fabric', subtitle: 'Modern renderer' },
  { id: '4', title: '📦 Hermes', subtitle: 'High-performance JS engine' },
];

function App(): React.JSX.Element {
  const renderItem = ({ item }: { item: DemoItem }) => (
    <View style={styles.item}>
      <Text style={styles.itemTitle}>{item.title}</Text>
      <Text style={styles.itemSubtitle}>{item.subtitle}</Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Welcome to {{PROJECT_NAME}} 🎉</Text>
      <Text style={styles.subtitle}>
        React Native running on HarmonyOS
      </Text>
      <FlatList
        style={styles.list}
        data={DEMO_ITEMS}
        keyExtractor={(item) => item.id}
        renderItem={renderItem}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: '#F5FCFF',
    paddingTop: 40,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    marginBottom: 8,
    textAlign: 'center',
  },
  subtitle: {
    fontSize: 16,
    color: '#666',
    marginBottom: 24,
    textAlign: 'center',
  },
  list: {
    flex: 1,
    width: '100%',
    paddingHorizontal: 16,
  },
  item: {
    backgroundColor: '#fff',
    padding: 16,
    borderRadius: 8,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#e0e0e0',
  },
  itemTitle: {
    fontSize: 18,
    fontWeight: '600',
    marginBottom: 4,
  },
  itemSubtitle: {
    fontSize: 14,
    color: '#888',
  },
});

export default App;
