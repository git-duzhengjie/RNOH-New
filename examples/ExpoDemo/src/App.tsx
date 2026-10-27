/**
 * ExpoDemo - App entry point
 *
 * Demonstrates Expo module compatibility on RNOH-New (HarmonyOS):
 *  a) expo-constants  — read Constants.expoConfig / platform / systemVersion
 *  b) expo-file-system — write file → read file → display content
 *  c) expo-secure-store — setItemAsync → getItemAsync → display token
 *  d) expo-asset — load asset and display (simplified to console demo)
 */

import React, { useEffect, useState } from 'react';
import {
  ScrollView,
  StyleSheet,
  Text,
  TouchableOpacity,
  View,
} from 'react-native';
import Constants from 'expo-constants';
// SDK 58 起 documentDirectory / cacheDirectory 等旧 API 移入 legacy 入口
import * as FileSystem from 'expo-file-system/legacy';
import * as SecureStore from 'expo-secure-store';
import { Asset } from 'expo-asset';

// ─── Types ───────────────────────────────────────────────────────────────────

interface DemoResult {
  title: string;
  status: 'idle' | 'running' | 'success' | 'error';
  output: string;
}

// ─── Demo: expo-constants ────────────────────────────────────────────────────

function runConstantsDemo(): string {
  const lines: string[] = [];

  const expoConfig = Constants.expoConfig;
  lines.push(`name: ${expoConfig?.name ?? '(undefined)'}`);
  lines.push(`slug: ${expoConfig?.slug ?? '(undefined)'}`);
  lines.push(`version: ${expoConfig?.version ?? '(undefined)'}`);
  lines.push(`sdkVersion: ${expoConfig?.sdkVersion ?? '(undefined)'}`);

  const platform = Constants.platform;
  lines.push(`platform: ${JSON.stringify(platform)}`);

  const systemVersion = Constants.systemVersion;
  lines.push(`systemVersion: ${systemVersion ?? '(undefined)'}`);

  const isDevice = Constants.isDevice;
  lines.push(`isDevice: ${isDevice}`);

  return lines.join('\n');
}

// ─── Demo: expo-file-system ──────────────────────────────────────────────────

async function runFileSystemDemo(): Promise<string> {
  const fileName = 'rnoh-demo.txt';
  const content = `Hello from RNOH-New!\nWritten at ${new Date().toISOString()}`;

  const baseDir = FileSystem.documentDirectory ?? FileSystem.cacheDirectory ?? '';
  if (!baseDir) {
    return 'documentDirectory / cacheDirectory 不可用（stub 环境）';
  }

  const fileUri = `${baseDir}${fileName}`;
  const lines: string[] = [];

  lines.push(`baseDir: ${baseDir}`);
  lines.push(`fileUri: ${fileUri}`);

  // Write
  await FileSystem.writeAsStringAsync(fileUri, content);
  lines.push(`writeAsStringAsync: OK (${content.length} bytes)`);

  // Read
  const readBack = await FileSystem.readAsStringAsync(fileUri);
  lines.push(`readAsStringAsync: OK`);
  lines.push(`content match: ${readBack === content}`);
  lines.push(`--- read content ---`);
  lines.push(readBack);

  return lines.join('\n');
}

// ─── Demo: expo-secure-store ─────────────────────────────────────────────────

async function runSecureStoreDemo(): Promise<string> {
  const key = 'rnoh_demo_token';
  const token = `tok_${Date.now()}_${Math.random().toString(36).slice(2, 10)}`;
  const lines: string[] = [];

  lines.push(`key: ${key}`);
  lines.push(`token to store: ${token}`);

  // Write
  await SecureStore.setItemAsync(key, token);
  lines.push(`setItemAsync: OK`);

  // Read
  const readBack = await SecureStore.getItemAsync(key);
  lines.push(`getItemAsync: OK`);
  lines.push(`token match: ${readBack === token}`);
  lines.push(`read back: ${readBack ?? '(null)'}`);

  return lines.join('\n');
}

// ─── Demo: expo-asset ────────────────────────────────────────────────────────

function runAssetDemo(): string {
  const lines: string[] = [];

  // Create an in-memory asset (no local file needed for demo)
  const asset = Asset.fromModule(1); // dummy module ID for demo
  lines.push(`Asset.fromModule(1): OK`);
  lines.push(`asset name: ${asset.name ?? '(undefined)'}`);
  lines.push(`asset type: ${asset.type ?? '(undefined)'}`);
  lines.push(`asset uri: ${asset.uri ?? '(undefined)'}`);
  lines.push(`asset localUri: ${asset.localUri ?? '(undefined)'}`);
  lines.push(`downloaded: ${asset.downloaded}`);
  lines.push(`hash: ${asset.hash ?? '(undefined)'}`);

  // Also log to console as required by the spec
  console.log('[ExpoDemo][expo-asset] asset demo:', {
    name: asset.name,
    type: asset.type,
    uri: asset.uri,
    localUri: asset.localUri,
    downloaded: asset.downloaded,
    hash: asset.hash,
  });

  return lines.join('\n');
}

// ─── Demo runner config ──────────────────────────────────────────────────────

interface DemoConfig {
  id: string;
  title: string;
  run: () => Promise<string> | string;
}

const DEMOS: DemoConfig[] = [
  { id: 'constants', title: 'expo-constants', run: runConstantsDemo },
  { id: 'file-system', title: 'expo-file-system', run: runFileSystemDemo },
  { id: 'secure-store', title: 'expo-secure-store', run: runSecureStoreDemo },
  { id: 'asset', title: 'expo-asset', run: runAssetDemo },
];

// ─── App ─────────────────────────────────────────────────────────────────────

function App(): React.JSX.Element {
  const [results, setResults] = useState<Record<string, DemoResult>>({});

  const runDemo = async (demo: DemoConfig) => {
    setResults((prev) => ({
      ...prev,
      [demo.id]: { title: demo.title, status: 'running', output: '运行中…' },
    }));

    try {
      const output = await demo.run();
      setResults((prev) => ({
        ...prev,
        [demo.id]: { title: demo.title, status: 'success', output },
      }));
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err);
      setResults((prev) => ({
        ...prev,
        [demo.id]: { title: demo.title, status: 'error', output: `Error: ${message}` },
      }));
    }
  };

  const runAll = async () => {
    for (const demo of DEMOS) {
      // eslint-disable-next-line no-await-in-loop
      await runDemo(demo);
    }
  };

  // Auto-run all demos on mount
  useEffect(() => {
    runAll();
  }, []);

  const statusColor = (status: DemoResult['status']) => {
    switch (status) {
      case 'running': return '#FF9800';
      case 'success': return '#4CAF50';
      case 'error': return '#F44336';
      default: return '#9E9E9E';
    }
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.title}>Expo 兼容性演示</Text>
      <Text style={styles.subtitle}>RNOH-New (React Native for HarmonyOS)</Text>

      {DEMOS.map((demo) => {
        const result = results[demo.id] ?? { title: demo.title, status: 'idle', output: '尚未运行' };
        return (
          <View key={demo.id} style={styles.card}>
            <View style={styles.cardHeader}>
              <Text style={styles.cardTitle}>{demo.title}</Text>
              <View style={[styles.badge, { backgroundColor: statusColor(result.status) }]}>
                <Text style={styles.badgeText}>{result.status}</Text>
              </View>
            </View>
            <Text style={styles.output}>{result.output}</Text>
            <TouchableOpacity style={styles.button} onPress={() => runDemo(demo)}>
              <Text style={styles.buttonText}>重新运行</Text>
            </TouchableOpacity>
          </View>
        );
      })}

      <TouchableOpacity style={styles.runAllButton} onPress={runAll}>
        <Text style={styles.runAllButtonText}>运行全部</Text>
      </TouchableOpacity>

      <View style={styles.footer} />
    </ScrollView>
  );
}

// ─── Styles ──────────────────────────────────────────────────────────────────

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F5FCFF',
  },
  content: {
    padding: 16,
    paddingTop: 40,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    textAlign: 'center',
    marginBottom: 4,
  },
  subtitle: {
    fontSize: 14,
    color: '#666',
    textAlign: 'center',
    marginBottom: 24,
  },
  card: {
    backgroundColor: '#fff',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#e0e0e0',
    padding: 16,
    marginBottom: 16,
  },
  cardHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  cardTitle: {
    fontSize: 18,
    fontWeight: '600',
  },
  badge: {
    paddingHorizontal: 8,
    paddingVertical: 2,
    borderRadius: 4,
  },
  badgeText: {
    color: '#fff',
    fontSize: 12,
    fontWeight: '600',
  },
  output: {
    fontSize: 13,
    color: '#333',
    fontFamily: 'monospace',
    marginBottom: 12,
    lineHeight: 20,
  },
  button: {
    alignSelf: 'flex-start',
    backgroundColor: '#2196F3',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 4,
  },
  buttonText: {
    color: '#fff',
    fontSize: 13,
    fontWeight: '600',
  },
  runAllButton: {
    backgroundColor: '#4CAF50',
    paddingVertical: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginBottom: 24,
  },
  runAllButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '700',
  },
  footer: {
    height: 40,
  },
});

export default App;
