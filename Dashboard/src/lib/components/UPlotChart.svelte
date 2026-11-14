<script>
  import { onMount } from 'svelte';
  import uPlot from 'uplot';
  import 'uplot/dist/uPlot.min.css';

  let {
    data = [[], []],
    series = [],
    height = 260,
    title = '',
  } = $props();

  let chartContainer = $state(null);
  let chartInstance = null;
  let chartWidth = $state(600);

  function buildOptions(width, h, seriesConfig) {
    const uSeries = [
      {
        value: (u, ts) => {
          if (!ts) return '--:--:--';
          const d = new Date(ts * 1000);
          return d.toLocaleTimeString();
        }
      },
      ...seriesConfig.map((s) => ({
        label: s.label || s.name,
        stroke: s.color || '#f59e0b',
        width: s.width || 1.8,
        fill: s.fill || null,
        value: (u, val) => (val != null ? `${Number(val).toFixed(s.decimals ?? 1)} ${s.unit ?? ''}` : '--'),
        points: { show: false },
      }))
    ];

    return {
      width,
      height: h,
      title: title || undefined,
      cursor: {
        sync: { key: 'telemetry_sync' },
        drag: { x: true, y: false },
        points: { size: 6, fill: '#f59e0b' }
      },
      scales: {
        x: { time: true },
      },
      axes: [
        {
          stroke: '#64748b',
          font: '10px monospace',
          grid: { stroke: '#e2e8f0', width: 1 },
          ticks: { stroke: '#cbd5e1', width: 1 },
          values: (u, splits) => splits.map(ts => new Date(ts * 1000).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }))
        },
        {
          stroke: '#64748b',
          font: '10px monospace',
          grid: { stroke: '#e2e8f0', width: 1 },
          ticks: { stroke: '#cbd5e1', width: 1 },
          size: 55,
        }
      ],
      series: uSeries,
    };
  }

  function initChart() {
    if (!chartContainer) return;
    const rect = chartContainer.getBoundingClientRect();
    const w = Math.max(300, rect.width || 600);
    chartWidth = w;

    if (chartInstance) {
      chartInstance.destroy();
      chartInstance = null;
    }

    const opts = buildOptions(w, height, series);
    chartInstance = new uPlot(opts, data, chartContainer);
  }

  onMount(() => {
    initChart();

    const ro = new ResizeObserver((entries) => {
      for (const entry of entries) {
        const newW = Math.floor(entry.contentRect.width);
        if (newW > 100 && chartInstance && Math.abs(newW - chartWidth) > 5) {
          chartWidth = newW;
          chartInstance.setSize({ width: newW, height });
        }
      }
    });

    if (chartContainer) {
      ro.observe(chartContainer);
    }

    return () => {
      ro.disconnect();
      if (chartInstance) {
        chartInstance.destroy();
        chartInstance = null;
      }
    };
  });

  $effect(() => {
    if (series && height && chartContainer) {
      initChart();
    }
  });

  $effect(() => {
    if (chartInstance && data && data.length > 0) {
      chartInstance.setData(data);
    }
  });
</script>

<div class="uplot-wrapper" bind:this={chartContainer}></div>

<style>
  .uplot-wrapper {
    width: 100%;
    min-height: 200px;
    background-color: var(--bg-card, #ffffff);
    border: 1px solid var(--border-medium, #e2e8f0);
    border-radius: var(--radius-sm, 2px);
    position: relative;
    overflow: hidden;
  }

  :global(.uplot) {
    font-family: var(--font-mono, monospace) !important;
  }

  :global(.uplot .u-legend) {
    font-size: 11px !important;
    background: var(--bg-panel-header, #f8fafc) !important;
    border-bottom: 1px solid var(--border-medium, #e2e8f0) !important;
    padding: 4px 10px !important;
  }

  :global(.uplot .u-legend th),
  :global(.uplot .u-legend td) {
    color: var(--text-primary, #0f172a) !important;
  }

  :global(.uplot .u-cursor-x),
  :global(.uplot .u-cursor-y) {
    border-color: rgba(245, 158, 11, 0.6) !important;
  }
</style>

