<script>
  import TelemetryLineChart from './TelemetryLineChart.svelte';
  import PowerSection from './PowerSection.svelte';
  import EnvironmentSection from './EnvironmentSection.svelte';
  import OtherSensorsSection from './OtherSensorsSection.svelte';

  let {
    telemetry = {},
    activeDevice = {},
    devices = [],
    isDeviceOnline = true,
    onCommand = () => {},
  } = $props();
</script>

<div class="stats-layout">
  <!-- Section 1: Real-time Telemetry Line Chart (uPlot Canvas High-Perf & SVG Native) -->
  <TelemetryLineChart
    {telemetry}
    activeDeviceId={activeDevice?.id || 'esp32_master'}
    {isDeviceOnline}
  />

  <!-- Section 2: Analitik Kelistrikan & Token PLN Prabayar -->
  <PowerSection {telemetry} {onCommand} />

  <!-- Section 3: Sensor Lingkungan, Tangki Air & Gas MQ -->
  <div class="stats-grid-2col">
    <OtherSensorsSection
      {telemetry}
      {devices}
      {onCommand}
    />
    <EnvironmentSection
      {telemetry}
      {isDeviceOnline}
      device={activeDevice}
    />
  </div>
</div>

<style>
  .stats-layout {
    display: flex;
    flex-direction: column;
    gap: 14px;
    padding-bottom: 24px;
  }

  .stats-grid-2col {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 14px;
  }

  @media (max-width: 1024px) {
    .stats-grid-2col {
      grid-template-columns: 1fr;
    }
  }
</style>

