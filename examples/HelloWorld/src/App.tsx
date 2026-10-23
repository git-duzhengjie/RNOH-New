/**
 * HelloWorld Example - App entry point
 */

import React from 'react';
import { AppRegistry, StyleSheet, Text, View } from 'react-native';

function App(): React.JSX.Element {
  return (
    <View style={styles.container}>
      <Text style={styles.title}>Hello from RNOH-New! 🎉</Text>
      <Text style={styles.subtitle}>
        React Native running on HarmonyOS
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: '#F5FCFF',
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    marginBottom: 16,
  },
  subtitle: {
    fontSize: 16,
    color: '#666',
  },
});

AppRegistry.registerComponent('HelloWorld', () => App);

export default App;
